/*
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 *
 * \brief SMSdb
 *
 * \author Max von Buelow <max@m9x.de>
 * \author Mark Spencer <markster@digium.com>
 *
 * The original code is from the astdb prat of the Asterisk project.
 */

#include <dirent.h>
#include <signal.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "ast_config.h"

#include <asterisk/app.h>
#include <asterisk/logger.h>

#include "smsdb.h"

#include "chan_quectel.h"

static const size_t DBKEY_DEF_LEN = 32;

#define DEFINE_SQL_STATEMENT(s, sql)      \
    static sqlite3_stmt* s##_stmt = NULL; \
    static const char s##_sql[]   = sql;

#define DEFINE_INTERNAL_SQL_STATEMENT(s, sql) static const char s##_sql[] = sql;

// OPER: incoming_msg
DEFINE_SQL_STATEMENT(get_incomingmsg, "SELECT message FROM incoming_msg WHERE key = ? ORDER BY seqorder")
DEFINE_SQL_STATEMENT(get_incomingmsg_cnt, "SELECT COUNT(seqorder) FROM incoming_msg WHERE key = ?")
DEFINE_SQL_STATEMENT(put_incomingmsg,
                     "INSERT OR REPLACE INTO incoming_msg (key, seqorder, expiration, message) "
                     "VALUES (?, ?, strftime('%s') + ?, ?)")
DEFINE_SQL_STATEMENT(del_incomingmsg, "DELETE FROM incoming_msg WHERE key = ?")

// OPER: outgoing_msg
DEFINE_SQL_STATEMENT(put_outgoingmsg, "INSERT INTO outgoing_msg (dev, dst, message, cnt, expiration, srr) VALUES (?, ?, ?, ?, strftime('%s') + ?, ?)")
DEFINE_SQL_STATEMENT(del_outgoingmsg, "DELETE FROM outgoing_msg WHERE uid = ?")
DEFINE_SQL_STATEMENT(get_outgoingmsg_key, "SELECT dev, dst, srr FROM outgoing_msg WHERE uid = ?")
DEFINE_SQL_STATEMENT(get_outgoingmsg, "SELECT dst, message FROM outgoing_msg WHERE uid = ?")
DEFINE_SQL_STATEMENT(get_outgoingmsg_expired, "SELECT uid, dst, message FROM outgoing_msg WHERE expiration < strftime('%s') LIMIT 1")

// OPER: outgoing_ref
DEFINE_SQL_STATEMENT(put_outgoingref, "INSERT INTO outgoing_ref (key) VALUES (?)")
DEFINE_SQL_STATEMENT(set_outgoingref, "UPDATE outgoing_ref SET refid = (refid + 1) % 256 WHERE key = ?")
DEFINE_SQL_STATEMENT(get_outgoingref, "SELECT refid FROM outgoing_ref WHERE key = ?")

// OPER: outgoing_part
DEFINE_SQL_STATEMENT(put_outgoingpart, "INSERT INTO outgoing_part (key, msg, status) VALUES (?, ?, NULL)")
DEFINE_SQL_STATEMENT(del_outgoingpart, "DELETE FROM outgoing_part WHERE msg = ?")
DEFINE_SQL_STATEMENT(set_outgoingpart, "UPDATE outgoing_part SET status = ? WHERE rowid = ?")
DEFINE_SQL_STATEMENT(get_outgoingpart, "SELECT rowid, msg FROM outgoing_part WHERE key = ?")
DEFINE_SQL_STATEMENT(get_all_status, "SELECT status FROM outgoing_part WHERE msg = ? ORDER BY rowid")

// OPER: outgoing_msg, outgoing_part
DEFINE_SQL_STATEMENT(cnt_outgoingpart,
                     "SELECT m.cnt, (SELECT COUNT(p.rowid) FROM outgoing_part p "
                     "               WHERE p.msg = m.rowid AND "
                     "               (p.status & 64 != 0 OR p.status & 32 = 0)) "
                     "FROM outgoing_msg m WHERE m.rowid = ?")
DEFINE_SQL_STATEMENT(cnt_all_outgoingpart,
                     "SELECT m.cnt, (SELECT COUNT(p.rowid) FROM outgoing_part p "
                     "               WHERE p.msg = m.uid) "
                     "FROM outgoing_msg m "
                     "WHERE m.uid = ?")

static sqlite3* smsdb;

static int set_ast_str(sqlite3_stmt* stmt, int colno, struct ast_str** str)
{
    if (!str || !*str) {
        return -1;
    }

    return ast_str_set(str, 0, "%.*s", sqlite3_column_bytes(stmt, colno), sqlite3_column_text(stmt, colno));
}

static int append_ast_str(sqlite3_stmt* stmt, int colno, struct ast_str** str)
{
    if (!str || !*str) {
        return -1;
    }

    return ast_str_append(str, 0, "%.*s", sqlite3_column_bytes(stmt, colno), sqlite3_column_text(stmt, colno));
}

static int bind_ast_str(sqlite3_stmt* stmt, int colno, const struct ast_str* const str)
{
    return sqlite3_bind_text(stmt, colno, ast_str_buffer(str), ast_str_strlen(str), SQLITE_TRANSIENT);
}

#if SQLITE_VERSION_NUMBER >= 3020000

//
// sqlite3_prepare_v3 function have been introduced in version 3.20.0 (2017-08-01) of SQLite
//
static int init_stmt(sqlite3_stmt** stmt, const char* sql, size_t len)
{
    const int res = sqlite3_prepare_v3(smsdb, sql, len, SQLITE_PREPARE_PERSISTENT, stmt, NULL);
    if (res != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't prepare statement '%s': [%d] %s\n", sql, res, sqlite3_errmsg(smsdb));
        return -1;
    }

    return 0;
}

#else

//
// sqlite3_prepare_v2 function have been introduced in version 3.3.9 (2007-01-04) of SQLite
//
static int init_stmt(sqlite3_stmt** stmt, const char* sql, size_t len)
{
    const int res = sqlite3_prepare_v2(smsdb, sql, len, stmt, NULL);
    if (res != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't prepare statement '%s': [%d] %s\n", sql, res, sqlite3_errmsg(smsdb));
        return -1;
    }

    return 0;
}

#endif

#define INIT_STMT(s) init_stmt(&s##_stmt, s##_sql, sizeof(s##_sql))
#define INIT_VSTMT(s) const int s##_res = init_stmt(&s##_stmt, s##_sql, sizeof(s##_sql))

/* We purposely don't lock around the sqlite3 call because the transaction
 * calls will be called with the database lock held. For any other use, make
 * sure to take the dblock yourself. */
static int execute_sql(const char* sql, int (*callback)(void*, int, char**, char**), void* arg)
{
    char* errmsg  = NULL;
    const int res = sqlite3_exec(smsdb, sql, callback, arg, &errmsg);

    if (res != SQLITE_OK) {
        ast_log(LOG_WARNING, "Error executing SQL (%s): [%d] %s\n", sql, res, errmsg);
        sqlite3_free(errmsg);
    }

    return res;
}

static int execute_ast_str(const struct ast_str* const str) { return execute_sql(ast_str_buffer(str), NULL, NULL); }

#define EXECUTE_STMT(s) execute_sql(s##_sql, NULL, NULL)
#define EXECUTE_VSTMT(s) const int s##_res = execute_sql(s##_sql, NULL, NULL)

/*! \internal
 * \brief Clean up the prepared SQLite3 statement
 * \note dblock should already be locked prior to calling this method
 */
static int clean_stmt(sqlite3_stmt** stmt, const char* sql)
{
    const int res = sqlite3_finalize(*stmt);
    if (res != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't finalize statement '%s': [%d] %s\n", sql, res, sqlite3_errmsg(smsdb));
        *stmt = NULL;
        return -1;
    }
    *stmt = NULL;
    return 0;
}

#define CLEAN_STMT(s) clean_stmt(&s##_stmt, s##_sql)

static int begin_transaction()
{
    DEFINE_INTERNAL_SQL_STATEMENT(begin_transaction, "BEGIN TRANSACTION");

    return EXECUTE_STMT(begin_transaction);
}

static void commit_transaction(int res)
{
    DEFINE_INTERNAL_SQL_STATEMENT(commit_transaction, "COMMIT TRANSACTION");

    if (res != SQLITE_OK) {
        return;
    }

    EXECUTE_STMT(commit_transaction);
}

#define SCOPED_TRANSACTION(varname)                                  \
    RAII_VAR(int, varname, begin_transaction(), commit_transaction); \
    if (varname != SQLITE_OK) return -1;

static sqlite3_stmt* stmt_begin(int level, const char* file, int line, const char* function, sqlite3_stmt* const stmt)
{
    if (!stmt) {
        ast_log(level, file, line, function, "Trying to begin uninitialized statement\n");
        return NULL;
    }

    const int res = sqlite3_clear_bindings(stmt);
    if (res != SQLITE_OK) {
        ast_log(level, file, line, function, "Fail to clear bindings: [%d] %s\n", res, sqlite3_errmsg(smsdb));
    }

    return stmt;
}

static void stmt_end(int level, const char* file, int line, const char* function, sqlite3_stmt* const stmt)
{
    if (!stmt) {
        return;
    }

    const int res = sqlite3_reset(stmt);
    if (res != SQLITE_OK) {
        ast_log(level, file, line, function, "Fail to reset statement: [%d] %s\n", res, sqlite3_errmsg(smsdb));
    }
}

#define SCOPED_STMT(s)                                                       \
    auto void _stmt_dtor_##s(sqlite3_stmt* const*);                          \
    void _stmt_dtor_##s(sqlite3_stmt* const* s) { stmt_end(LOG_ERROR, *s); } \
    sqlite3_stmt* const s __attribute__((cleanup(_stmt_dtor_##s))) = stmt_begin(LOG_WARNING, s##_stmt)

static sqlite3_mutex* db_lock(sqlite3* db)
{
    //
    // sqlite3_db_mutex function have been introduced in version 3.6.5 (2008-11-12) of SQLite
    //
    sqlite3_mutex* const mtx = sqlite3_db_mutex(db);
    if (!mtx) {
        return NULL;
    }
    sqlite3_mutex_enter(mtx);
    return mtx;
}

static void db_unlock(sqlite3_mutex* const mtx)
{
    if (!mtx) {
        return;
    }
    sqlite3_mutex_leave(mtx);
}

#define SCOPED_DB(d) RAII_VAR(sqlite3_mutex*, d##_mtx, db_lock(d), db_unlock)

static int db_create(void)
{
    // TABLE: incoming_msg
    DEFINE_INTERNAL_SQL_STATEMENT(create_incomingmsg,
                                  "CREATE TABLE IF NOT EXISTS incoming_msg (key VARCHAR(256), seqorder INTEGER,"
                                  "expiration TIMESTAMP DEFAULT (strftime('%s')), message VARCHAR(256), PRIMARY KEY(key, seqorder))")
    DEFINE_INTERNAL_SQL_STATEMENT(create_incomingmsg_index, "CREATE INDEX IF NOT EXISTS incoming_key ON incoming_msg(key)")

    // TABLE: outgoing_msg(KEY: IMSI/DEST_ADDR)
    DEFINE_INTERNAL_SQL_STATEMENT(create_outgoingmsg,
                                  "CREATE TABLE IF NOT EXISTS outgoing_msg (uid INTEGER PRIMARY KEY AUTOINCREMENT,"
                                  "dev VARCHAR(256), dst VARCHAR(256), message VARCHAR(256), cnt INTEGER, expiration TIMESTAMP, srr BOOLEAN)")

    // TABLE: outgoing_ref
    DEFINE_INTERNAL_SQL_STATEMENT(create_outgoingref, "CREATE TABLE IF NOT EXISTS outgoing_ref (key VARCHAR(256), refid INTEGER DEFAULT 0, PRIMARY KEY(key))")

    // TABLE: outgoing_part(KEY: IMSI/DEST_ADDR/MR)
    DEFINE_INTERNAL_SQL_STATEMENT(create_outgoingpart,
                                  "CREATE TABLE IF NOT EXISTS outgoing_part (key VARCHAR(256), msg INTEGER, status INTEGER, PRIMARY KEY(key))")
    DEFINE_INTERNAL_SQL_STATEMENT(create_outgoingpart_index, "CREATE INDEX IF NOT EXISTS outgoing_part_msg ON outgoing_part(msg)")

    SCOPED_DB(smsdb);
    SCOPED_TRANSACTION(dbtrans);

    EXECUTE_VSTMT(create_incomingmsg);
    EXECUTE_VSTMT(create_incomingmsg_index);
    EXECUTE_VSTMT(create_outgoingmsg);
    EXECUTE_VSTMT(create_outgoingref);
    EXECUTE_VSTMT(create_outgoingpart);
    EXECUTE_VSTMT(create_outgoingpart_index);

    return create_incomingmsg_res || create_incomingmsg_index_res || create_outgoingmsg_res || create_outgoingref_res || create_outgoingpart_res ||
           create_outgoingpart_index_res;
}

static int db_init_statements(void)
{
    /* Don't initialize create_smsdb_statement here as the smsdb table needs to exist
     * brefore these statements can be initialized */

    SCOPED_DB(smsdb);

    INIT_VSTMT(get_incomingmsg);
    INIT_VSTMT(put_incomingmsg);
    INIT_VSTMT(del_incomingmsg);
    INIT_VSTMT(get_incomingmsg_cnt);
    INIT_VSTMT(put_outgoingref);
    INIT_VSTMT(set_outgoingref);
    INIT_VSTMT(get_outgoingref);
    INIT_VSTMT(put_outgoingmsg);
    INIT_VSTMT(put_outgoingpart);
    INIT_VSTMT(del_outgoingmsg);
    INIT_VSTMT(del_outgoingpart);
    INIT_VSTMT(get_outgoingmsg_key);
    INIT_VSTMT(get_outgoingpart);
    INIT_VSTMT(set_outgoingpart);
    INIT_VSTMT(cnt_outgoingpart);
    INIT_VSTMT(cnt_all_outgoingpart);
    INIT_VSTMT(get_outgoingmsg);
    INIT_VSTMT(get_all_status);
    INIT_VSTMT(get_outgoingmsg_expired);

    return get_incomingmsg_res || put_incomingmsg_res || del_incomingmsg_res || get_incomingmsg_cnt_res || put_outgoingref_res || set_outgoingref_res ||
           get_outgoingref_res || put_outgoingmsg_res || put_outgoingpart_res || del_outgoingmsg_res || del_outgoingpart_res || get_outgoingmsg_key_res ||
           get_outgoingpart_res || set_outgoingpart_res || cnt_outgoingpart_res || cnt_all_outgoingpart_res || get_outgoingmsg_res || get_all_status_res ||
           get_outgoingmsg_expired_res;
}

static void db_clean_statements(void)
{
    SCOPED_DB(smsdb);

    CLEAN_STMT(get_incomingmsg);
    CLEAN_STMT(get_incomingmsg_cnt);
    CLEAN_STMT(put_incomingmsg);
    CLEAN_STMT(del_incomingmsg);
    CLEAN_STMT(put_outgoingref);
    CLEAN_STMT(set_outgoingref);
    CLEAN_STMT(get_outgoingref);
    CLEAN_STMT(put_outgoingmsg);
    CLEAN_STMT(put_outgoingpart);
    CLEAN_STMT(del_outgoingmsg);
    CLEAN_STMT(del_outgoingpart);
    CLEAN_STMT(get_outgoingmsg_key);
    CLEAN_STMT(set_outgoingpart);
    CLEAN_STMT(get_outgoingpart);
    CLEAN_STMT(cnt_outgoingpart);
    CLEAN_STMT(cnt_all_outgoingpart);
    CLEAN_STMT(get_outgoingmsg);
    CLEAN_STMT(get_all_status);
    CLEAN_STMT(get_outgoingmsg_expired);
}

static int db_name_in_memory(const char* db)
{
    static const char SQLITE_IN_MEMORY_SPECIAL_NAME[] = ":memory:";

    if (!db) {
        return 0;
    }
    return !strncmp(db, SQLITE_IN_MEMORY_SPECIAL_NAME, STRLEN(SQLITE_IN_MEMORY_SPECIAL_NAME));
}

static int db_name_temporary(const char* db)
{
    static const char SQLITE_TMP_SPECIAL_NAME[] = ":temporary:";

    if (!db) {
        return 0;
    }
    return !strncmp(db, SQLITE_TMP_SPECIAL_NAME, STRLEN(SQLITE_TMP_SPECIAL_NAME));
}

static int db_open_url(const char* url)
{
    //
    // sqlite3_open_v2 function have been introduced in version 3.5.0 (2007-09-04) of SQLite
    //
    const int res = sqlite3_open_v2(url, &smsdb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
    if (res != SQLITE_OK) {
        ast_log(LOG_WARNING, "Unable to open database '%s': [%d] %s\n", url, res, sqlite3_errmsg(smsdb));
        sqlite3_close(smsdb);
        return -1;
    }

    return 0;
}

static int db_open(void)
{
    static const size_t DBNAME_DEF_LEN = 32;

    if (db_name_in_memory(CONF_GLOBAL(sms_db))) {
        return db_open_url(CONF_GLOBAL(sms_db));
    } else if (db_name_temporary(CONF_GLOBAL(sms_db))) {
        return db_open_url("");
    } else {
        RAII_VAR(struct ast_str*, dbname, ast_str_create(DBNAME_DEF_LEN), ast_free);
        ast_str_set(&dbname, 0, "%s.sqlite3", CONF_GLOBAL(sms_db));
        return db_open_url(ast_str_buffer(dbname));
    }
}

static int db_init()
{
    if (smsdb) {
        return 0;
    }

    if (db_open() || db_create() || db_init_statements()) {
        return -1;
    }

    return 0;
}

static int put_incmsg(const struct ast_str* const fullkey, int order, const char* msg)
{
    int res;
    const int ttl = CONF_GLOBAL(sms_ttl);

    SCOPED_DB(smsdb);
    SCOPED_STMT(put_incomingmsg);

    if ((res = bind_ast_str(put_incomingmsg, 1, fullkey)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_bind_int(put_incomingmsg, 2, order)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind order to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_bind_int(put_incomingmsg, 3, ttl)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind TTL to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_bind_text(put_incomingmsg, 4, msg, -1, SQLITE_TRANSIENT)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind msg to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(put_incomingmsg)) != SQLITE_DONE) {
        ast_log(LOG_WARNING, "Couldn't execute statement: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    return 0;
}

static int get_incmsg_cnt(const struct ast_str* const fullkey)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(get_incomingmsg_cnt);

    if ((res = bind_ast_str(get_incomingmsg_cnt, 1, fullkey)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(get_incomingmsg_cnt)) != SQLITE_ROW) {
        ast_debug(1, "Unable to find key '%s'\n", ast_str_buffer(fullkey));
        return -1;
    }

    return sqlite3_column_int(get_incomingmsg_cnt, 0);
}

static int get_incmsg(const struct ast_str* const fullkey, struct ast_str** out)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(get_incomingmsg);

    if ((res = bind_ast_str(get_incomingmsg, 1, fullkey)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    while ((res = sqlite3_step(get_incomingmsg)) == SQLITE_ROW) {
        append_ast_str(get_incomingmsg, 0, out);
    }

    return (res == SQLITE_DONE) ? 0 : -1;
}

static int del_incmsg(const struct ast_str* const fullkey)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(del_incomingmsg);

    if ((res = bind_ast_str(del_incomingmsg, 1, fullkey)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(del_incomingmsg)) != SQLITE_DONE) {
        ast_debug(1, "Unable to find key '%s'; Ignoring\n", ast_str_buffer(fullkey));
        return 0;
    }

    return 0;
}

/*!
 * \brief Adds a message part into the DB and returns the whole message into 'out' when the message is complete.
 * \param id -- Some ID for the device or so, e.g. the IMSI
 * \param addr -- The sender address
 * \param ref -- The reference ID
 * \param parts -- The total number of messages
 * \param order -- The current message number
 * \param msg -- The current message part
 * \param out -- Output: Only written if parts == cnt
 * \retval <=0 Error
 * \retval >0 Current number of messages in the DB
 */
int smsdb_put(const char* id, const char* addr, int ref, int parts, int order, const char* msg, struct ast_str** out)
{
    RAII_VAR(struct ast_str*, fullkey, ast_str_create(DBKEY_DEF_LEN), ast_free);
    if (ast_str_set(&fullkey, 0, "%s/%s/%d/%d", id, addr, ref, parts) < 0) {
        ast_log(LOG_ERROR, "Fail to create key\n");
        return -1;
    }

    int res;
    SCOPED_TRANSACTION(dbtrans);

    if ((res = put_incmsg(fullkey, order, msg))) {
        return res;
    }

    if ((res = get_incmsg_cnt(fullkey)) == parts) {
        if (get_incmsg(fullkey, out)) {
            ast_str_reset(*out);
        } else {
            del_incmsg(fullkey);
        }
    }

    return res;
}

static int get_refid(const struct ast_str* const fullkey)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(get_outgoingref);

    if ((res = bind_ast_str(get_outgoingref, 1, fullkey)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    switch (res = sqlite3_step(get_outgoingref)) {
        case SQLITE_ROW:
            return sqlite3_column_int(get_outgoingref, 0);

        case SQLITE_DONE:
            return -2;

        default:
            return -1;
    }
}

static int inc_refid(const struct ast_str* const fullkey, int use_insert)
{
    int res;

    sqlite3_stmt* const outgoingref_stmt = use_insert ? put_outgoingref_stmt : set_outgoingref_stmt;

    SCOPED_DB(smsdb);
    SCOPED_STMT(outgoingref);

    if ((res = bind_ast_str(outgoingref, 1, fullkey)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(outgoingref)) != SQLITE_DONE) {
        return -1;
    }

    return 0;
}

int smsdb_get_refid(const char* id, const char* addr)
{
    RAII_VAR(struct ast_str*, fullkey, ast_str_create(DBKEY_DEF_LEN), ast_free);
    if (ast_str_set(&fullkey, 0, "%s/%s", id, addr) < 0) {
        ast_log(LOG_ERROR, "Fail to create key\n");
        return -1;
    }

    int res, use_insert = 0;

    SCOPED_TRANSACTION(dbtrans);

    if ((res = get_refid(fullkey)) == -2) {
        res        = 0;
        use_insert = 1;
    }

    if (res < 0) {
        return res;
    }

    inc_refid(fullkey, use_insert);
    return (res + 1) % 256;
}

int smsdb_outgoing_add(const char* id, const char* addr, const char* msg, int cnt, int ttl, int srr)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_TRANSACTION(dbtrans);
    SCOPED_STMT(put_outgoingmsg);

    if ((res = sqlite3_bind_text(put_outgoingmsg, 1, id, strlen(id), SQLITE_TRANSIENT)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind dev to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_bind_text(put_outgoingmsg, 2, addr, strlen(addr), SQLITE_TRANSIENT)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind destination address to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_bind_text(put_outgoingmsg, 3, msg, strlen(msg), SQLITE_TRANSIENT)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind message to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_bind_int(put_outgoingmsg, 4, cnt)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind count to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_bind_int(put_outgoingmsg, 5, ttl)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind TTL to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_bind_int(put_outgoingmsg, 6, srr)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind SRR to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(put_outgoingmsg)) != SQLITE_DONE) {
        return -1;
    }

    return sqlite3_last_insert_rowid(smsdb);
}

static int get_outmsg(int uid, struct ast_str** dst, struct ast_str** msg)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(get_outgoingmsg);

    if ((res = sqlite3_bind_int(get_outgoingmsg, 1, uid)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(get_outgoingmsg)) != SQLITE_ROW) {
        return -1;
    }

    res = set_ast_str(get_outgoingmsg, 0, dst);
    res = set_ast_str(get_outgoingmsg, 1, msg);

    return 0;
}

static int del_outmsg(int uid)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(del_outgoingmsg);

    if ((res = sqlite3_bind_int(del_outgoingmsg, 1, uid)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind UID to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(del_outgoingmsg)) != SQLITE_DONE) {
        return -1;
    }

    return 0;
}

static int del_outpart(int uid)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(del_outgoingpart);

    if ((res = sqlite3_bind_int(del_outgoingpart, 1, uid)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind UID to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(del_outgoingpart)) != SQLITE_DONE) {
        return -1;
    }

    return 0;
}

static int smsdb_outgoing_clear_nolock(int uid)
{
    if (del_outmsg(uid)) {
        return -1;
    }

    if (del_outpart(uid)) {
        return -1;
    }

    return 0;
}

int smsdb_outgoing_clear(int uid, struct ast_str** dst, struct ast_str** msg)
{
    SCOPED_TRANSACTION(dbtrans);

    if (get_outmsg(uid, dst, msg)) {
        return -1;
    }

    if (smsdb_outgoing_clear_nolock(uid)) {
        return -1;
    }

    return 0;
}

static int get_outmsg_key(int uid, int refid, struct ast_str** fullkey, int* srr)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(get_outgoingmsg_key);

    if ((res = sqlite3_bind_int(get_outgoingmsg_key, 1, uid)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind UID to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(get_outgoingmsg_key)) != SQLITE_ROW) {
        return -2;
    }

    const char* dev = (const char*)sqlite3_column_text(get_outgoingmsg_key, 0);
    const char* dst = (const char*)sqlite3_column_text(get_outgoingmsg_key, 1);
    *srr            = sqlite3_column_int(get_outgoingmsg_key, 2);

    if (ast_str_set(fullkey, 0, "%s/%s/%d", dev, dst, refid) < 0) {
        ast_log(LOG_ERROR, "Fail to create key\n");
        return -3;
    }

    return 0;
}

static int put_outpart(const struct ast_str* const fullkey, int uid)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(put_outgoingpart);

    if ((res = bind_ast_str(put_outgoingpart, 1, fullkey)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_bind_int(put_outgoingpart, 2, uid)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind UID to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(put_outgoingpart)) != SQLITE_DONE) {
        return -1;
    }

    return 0;
}

static int cnt_all_outpart(int uid)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(cnt_all_outgoingpart);

    if ((res = sqlite3_bind_int(cnt_all_outgoingpart, 1, uid)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(cnt_all_outgoingpart)) != SQLITE_ROW) {
        return -1;
    }

    const int cur = sqlite3_column_int(cnt_all_outgoingpart, 0);
    const int cnt = sqlite3_column_int(cnt_all_outgoingpart, 1);

    if (cur != cnt) {
        return -2;
    }

    return 0;
}

int smsdb_outgoing_part_put(int uid, int refid, struct ast_str** dst, struct ast_str** msg)
{
    int srr = -1;

    RAII_VAR(struct ast_str*, fullkey, ast_str_create(DBKEY_DEF_LEN), ast_free);

    SCOPED_TRANSACTION(dbtrans);

    if (get_outmsg_key(uid, refid, &fullkey, &srr)) {
        return -1;
    }

    if (put_outpart(fullkey, uid)) {
        return -1;
    }

    if (!srr) {
        return -2;
    }

    if (cnt_all_outpart(uid)) {
        return -1;
    }

    if (get_outmsg(uid, dst, msg)) {
        return -1;
    }

    // clear if everything is finished
    if (smsdb_outgoing_clear_nolock(uid)) {
        return -1;
    }

    return 0;
}

static int get_outpart(const struct ast_str* const fullkey, int* partid, int* uid)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(get_outgoingpart);

    if ((res = bind_ast_str(get_outgoingpart, 1, fullkey)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(get_outgoingpart)) != SQLITE_ROW) {
        return -1;
    }

    *partid = sqlite3_column_int(get_outgoingpart, 0);
    *uid    = sqlite3_column_int(get_outgoingpart, 1);
    return 0;
}

static int set_outpart(int st, int partid)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(set_outgoingpart);

    if ((res = sqlite3_bind_int(set_outgoingpart, 1, st)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind status to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_bind_int(set_outgoingpart, 2, partid)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind ID to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(set_outgoingpart)) != SQLITE_DONE) {
        return -1;
    }

    return 0;
}

static int cnt_outpart(int uid)
{
    int res;

    SCOPED_DB(smsdb);
    SCOPED_STMT(cnt_outgoingpart);

    if ((res = sqlite3_bind_int(cnt_outgoingpart, 1, uid)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    if ((res = sqlite3_step(cnt_outgoingpart)) != SQLITE_ROW) {
        return -1;
    }

    const int cur = sqlite3_column_int(cnt_outgoingpart, 0);
    const int cnt = sqlite3_column_int(cnt_outgoingpart, 1);
    if (cur != cnt) {
        return -2;
    }

    return 0;
}

static int get_all_stats(int uid, int* status_all)
{
    int res, i = 0;

    SCOPED_DB(smsdb);
    SCOPED_STMT(get_all_status);

    if ((res = sqlite3_bind_int(get_all_status, 1, uid)) != SQLITE_OK) {
        ast_log(LOG_WARNING, "Couldn't bind key to stmt: [%d] %s\n", res, sqlite3_errmsg(smsdb));
        return -1;
    }

    while ((res = sqlite3_step(get_all_status)) == SQLITE_ROW) {
        status_all[i++] = sqlite3_column_int(get_all_status, 0);
    }
    status_all[i] = -1;
    return (res == SQLITE_DONE) ? 0 : -1;
}

int smsdb_outgoing_part_status(const char* id, const char* addr, int mr, int st, int* status_all)
{
    int partid = -1, uid = -1;

    RAII_VAR(struct ast_str*, fullkey, ast_str_create(DBKEY_DEF_LEN), ast_free);
    if (ast_str_set(&fullkey, 0, "%s/%s/%d", id, addr, mr) < 0) {
        ast_log(LOG_ERROR, "Fail to create key\n");
        return -1;
    }

    SCOPED_TRANSACTION(dbtrans);

    if (get_outpart(fullkey, &partid, &uid)) {
        return -1;
    }

    if (set_outpart(st, partid)) {
        return -1;
    }

    if (cnt_outpart(uid)) {
        return -1;
    }

    if (get_all_stats(uid, status_all)) {
        return -1;
    }

    // clear if everything is finished
    if (smsdb_outgoing_clear_nolock(uid)) {
        return -1;
    }

    return 0;
}

static int get_outmsg_expired(int* uid, struct ast_str** dst, struct ast_str** msg)
{
    int res;

    SCOPED_STMT(get_outgoingmsg_expired);

    if ((res = sqlite3_step(get_outgoingmsg_expired)) != SQLITE_ROW) {
        return -1;
    }

    *uid = sqlite3_column_int(get_outgoingmsg_expired, 0);
    res  = set_ast_str(get_outgoingmsg_expired, 1, dst);
    res  = set_ast_str(get_outgoingmsg_expired, 2, msg);
    return 0;
}

int smsdb_outgoing_purge_one(int* uid, struct ast_str** dst, struct ast_str** msg)
{
    SCOPED_TRANSACTION(dbtrans);

    if (get_outmsg_expired(uid, dst, msg)) {
        return -1;
    }

    if (smsdb_outgoing_clear_nolock(*uid)) {
        return -1;
    }

    return 0;
}

int smsdb_vacuum_into(const char* backup_file)
{
    static const size_t SQLSTMT_DEF_LEN = 64;

    if (ast_file_is_readable(backup_file)) {
        remove(backup_file);
    }

    RAII_VAR(struct ast_str*, sqlstmt, ast_str_create(SQLSTMT_DEF_LEN), ast_free);
    ast_str_set(&sqlstmt, 0, "VACUUM INTO \"%s\"", backup_file);

    return execute_ast_str(sqlstmt);
}

/*!
 * \internal
 * \brief Clean up resources on Asterisk shutdown
 */
void smsdb_atexit()
{
    db_clean_statements();
    if (sqlite3_close(smsdb) == SQLITE_OK) {
        smsdb = NULL;
    }
}

int smsdb_init()
{
    if (db_init()) {
        return -1;
    }

    return 0;
}
