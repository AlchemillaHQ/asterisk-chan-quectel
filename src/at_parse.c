/*
   Copyright (C) 2009 - 2010

   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org

   Dmitry Vagin <dmitry2004@yandex.ru>

   bg <bg_one@mail.ru>
*/
#include <errno.h>  /* errno */
#include <stdio.h>  /* NULL */
#include <stdlib.h> /* strtol */

#include "ast_config.h"

#include "at_parse.h"

#include "chan_quectel.h"
#include "error.h"
#include "memmem.h"
#include "mutils.h" /* ARRAY_LEN() */
#include "pdu.h"    /* pdu_parse() */

#/* */

static unsigned mark_line(char* line, const char* delimiters, char* pointers[])
{
    unsigned found = 0;

    for (; line[0] && delimiters[found]; line++) {
        if (line[0] == delimiters[found]) {
            pointers[found] = line;
            found++;
        }
    }
    return found;
}

static void trim_line(char* const pointers[], unsigned cnt)
{
    for (unsigned i = 0; i < cnt; ++i) {
        pointers[i][0] = '\000';
    }
}

static char* strip_quoted(char* buf) { return ast_strip_quoted(buf, "\"", "\""); }

static char* trim_blanks(char* str)
{
    char* work = str;

    if (work) {
        work += strlen(work) - 1;
        while ((work >= str) && (((unsigned char)*work) < 33 || *work == '@' || ((unsigned char)*work) >= 128)) {
            *(work--) = '\0';
        }
    }
    return str;
}

const char* attribute_const at_qind2str(qind_t qind)
{
    static const char* qind_names[] = {"NONE", "CSQ", "ACT", "CCINFO"};
    return enum2str_def(qind, qind_names, ARRAY_LEN(qind_names), "UNK");
}

/*!
 * \brief Parse a CNUM response
 * \param str -- string to parse (null terminated)
 * @note str will be modified when the CNUM message is parsed
 * \return NULL on error (parse error) or a pointer to the subscriber number
 */

char* at_parse_cnum(char* str)
{
    /*
     * parse CNUM response in the following format:
     * +CNUM: <name>,<number>,<type>
     *   example
     *   +CNUM: "Subscriber Number","+79139131234",145
     *   +CNUM: "Subscriber Number","",145
     *   +CNUM: "Subscriber Number",,145
     */

    static char delimiters[] = ":,,";
    char* marks[STRLEN(delimiters)];

    /* parse URC only here */
    if (mark_line(str, delimiters, marks) != ARRAY_LEN(marks)) {
        return NULL;
    }

    trim_line(marks, ARRAY_LEN(marks));
    return strip_quoted(marks[1] + 1);
}

/*!
 * \brief Parse a COPS response
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * @note str will be modified when the COPS message is parsed
 * \return NULL on error (parse error) or a pointer to the provider name
 */
char* at_parse_cops(char* str)
{
    /*
     * parse COPS response in the following format:
     * +COPS: <mode>[,<format>,<oper>,<?>]
     *
     * example
     *  +COPS: 0,0,"TELE2",0
     *  +COPS: 0,0,"POL"
     */

    static const char delimiters[]     = ":,,,";
    static const size_t delimiters_no  = STRLEN(delimiters);
    static const size_t delimiters_no1 = delimiters_no - 1u;

    char* marks[delimiters_no];

    /* parse URC only here */
    const unsigned int marks_no = mark_line(str, delimiters, marks);
    if (marks_no >= delimiters_no1) {
        if (marks_no > 3) {
            marks[3][0] = '\000';
        }
        char* res = strip_quoted(marks[2] + 1);
        /* Sometimes there is trailing garbage here;
         * e.g. "Tele2@" or "Tele2<U+FFFD>" instead of "Tele2".
         * Unsure why it happens (provider? quectel?), but it causes
         * trouble later on too (at pbx_builtin_setvar_helper which
         * is not encoding agnostic anymore, now that it uses json
         * for messaging). See wdoekes/asterisk-chan-quectel
         * GitHub issues #39 and #69. */
        res = trim_blanks(res);
        return res;
    }

    return NULL;
}

int at_parse_qspn(char* str, char** fnn, char** snn, char** spn)
{
    /*
        +QSPN: <FNN>,<SNN>,<SPN>,<alphabet>,<RPLMN>

        examples:

        +QSPN: "aaaaa","bbbbb","ccccc",0,"000000"
    */

    static const char delimiters[] = ":,,,,";
    char* marks[STRLEN(delimiters)];

    /* parse URC only here */
    if (mark_line(str, delimiters, marks) == ARRAY_LEN(marks)) {
        marks[0]++;
        if (marks[0][0] == ' ') {
            marks[0]++;
        }

        marks[1][0] = '\000';
        marks[1]++;

        marks[2][0] = '\000';
        marks[2]++;

        marks[3][0] = '\000';

        *spn = strip_quoted(marks[2]);
        *snn = strip_quoted(marks[1]);
        *fnn = strip_quoted(marks[0]);
        return 0;
    }

    return -1;
}

int at_parse_cspn(char* str, char** spn)
{
    /*
        +CSPN: <spn>,<display mode>
    */

    static const char delimiters[] = ":,";
    char* marks[STRLEN(delimiters)];

    if (mark_line(str, delimiters, marks) == ARRAY_LEN(marks)) {
        marks[0]++;

        marks[1][0] = '\000';
        marks[1]++;

        *spn = strip_quoted(marks[0]);
        return 0;
    }

    return -1;
}

static int attribute_const act2int(const char* act)
{
    static const struct {
        const char* act;
        int val;
    } ACTS[] = {
        {"NONE",             0 },
        {"UNKNOWN",          0 },
        {"GSM",              1 },
        {"GPRS",             2 },
        {"EDGE",             3 },
        {"EGPRS",            3 },
        {"WCDMA",            4 },
        {"HSDPA",            5 },
        {"HSUPA",            6 },
        {"HSPA+",            7 },
        {"HSDPA&HSUPA",      7 },
        {"TDD LTE",          8 },
        {"FDD LTE",          8 },
        {"LTE",              8 },
        {"TDSCDMA",          9 },
        {"TD-SCDMA",         9 },
        {"CDMA1X",           13},
        {"CDMA",             13},
        {"EVDO",             14},
        {"CDMA1X AND HDR",   15},
        {"HDR",              16},
        {"CDMA1X AND EHRPD", 24},
        {"HDR-EHRPD",        24},
    };

    for (size_t idx = 0; idx < ARRAY_LEN(ACTS); ++idx) {
        if (!strcmp(ACTS[idx].act, act)) {
            return ACTS[idx].val;
        }
    }

    return -1;
}

int at_parse_qnwinfo(char* str, int* act, int* oper, char** band, int* channel)
{
    /*
        +QNWINFO: <Act>,<oper>,<band>,<channel>
        +QNWINFO: No Service
    */

    static const char delimiters[] = ":,,,";
    static const char NO_SERVICE[] = "No Service";

    char* marks[STRLEN(delimiters)];

    /* parse URC only here */
    const int nmarks = mark_line(str, delimiters, marks);
    if (nmarks == ARRAY_LEN(marks)) {
        marks[0]++;
        if (marks[0][0] == ' ') {
            marks[0]++;
        }

        marks[1][0] = '\000';
        marks[1]++;

        marks[2][0] = '\000';
        marks[2]++;

        marks[3][0] = '\000';
        marks[3]++;

        const long ch = strtol(marks[3], (char**)NULL, 10);
        if (!ch) {
            return -1;
        }
        *channel = (int)ch;

        *band = strip_quoted(marks[2]);

        const long o = strtol(strip_quoted(marks[1]), (char**)NULL, 10);
        if (!o) {
            return -1;
        }
        *oper = (int)o;

        *act = act2int(strip_quoted(marks[0]));
        return 0;
    } else if (nmarks == 1) {
        const char* s = ast_strip(marks[0] + 1);
        if (!strncmp(s, NO_SERVICE, STRLEN(NO_SERVICE))) {
            *act = -1;
            return 0;
        }
    }

    return -1;
}

/*!
 * \brief Parse a C(E)REG response
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \param gsm_reg -- a pointer to a int
 * \param gsm_reg_status -- a pointer to a int
 * \param lac -- a pointer to a char pointer which will store the location area code in hex format
 * \param ci  -- a pointer to a char pointer which will store the cell id in hex format
 * \param act -- a pointer to an integer which will store access technology
 * @note str will be modified when the C(E)REG message is parsed
 * \retval  0 success
 * \retval -1 parse error
 */
int at_parse_creg(char* str, int* gsm_reg, int* gsm_reg_status, char** lac, char** ci, int* act)
{
    char* gsm_reg_str = NULL;

    *gsm_reg            = 0;
    *gsm_reg_status     = -1;
    *lac                = NULL;
    *ci                 = NULL;
    *act                = -1;
    const char* act_str = NULL;

    /*
     * parse C(E)REG response in the following formats:
     *
     *   +CREG: <n>,<stat>[,<LAC>,<ci>[,<Act>]] - response to AT+CREG?
     *   +CREG: <stat>[,<LAC>,<ci>[,<Act>]]     - URC after AT-CREG=2
     *
     *   +CEREG: <stat>[,<tac>,<ci>[,<AcT>]]
     */

    static const char delimiters[] = ":,,,,";

    char* marks[STRLEN(delimiters)];
    const unsigned int n = mark_line(str, delimiters, marks);
    switch (n) {
        case 5:
            marks[1][0] = '\000';
            marks[2][0] = '\000';
            marks[3][0] = '\000';
            marks[4][0] = '\000';
            act_str     = marks[4] + 1;
            *ci         = strip_quoted(marks[3] + 1);
            *lac        = strip_quoted(marks[2] + 1);
            gsm_reg_str = strip_quoted(marks[1] + 1);
            break;

        case 4:
            marks[1][0] = '\000';
            marks[2][0] = '\000';
            marks[3][0] = '\000';
            if (marks[1][1] == '"') {
                *ci         = strip_quoted(marks[2] + 1);
                *lac        = strip_quoted(marks[1] + 1);
                gsm_reg_str = strip_quoted(marks[0] + 1);
            } else {
                act_str     = marks[3] + 1;
                *ci         = strip_quoted(marks[3] + 1);
                *lac        = strip_quoted(marks[2] + 1);
                gsm_reg_str = strip_quoted(marks[1] + 1);
            }
            break;

        case 3:
            marks[1][0] = '\000';
            marks[2][0] = '\000';
            *ci         = strip_quoted(marks[2] + 1);
            *lac        = strip_quoted(marks[1] + 1);
            gsm_reg_str = strip_quoted(marks[0] + 1);
            break;

        case 2:
            marks[1][0] = '\000';
            gsm_reg_str = strip_quoted(marks[1] + 1);
            break;

        case 1:
            gsm_reg_str = strip_quoted(marks[0] + 1);
            break;
    }

    if (gsm_reg_str) {
        errno            = 0;
        const int status = (int)strtol(gsm_reg_str, (char**)NULL, 10);
        if (status == 0 && errno == EINVAL) {
            *gsm_reg_status = -1;
            return -1;
        }

        *gsm_reg_status = status;
        if (status == 1 || status == 5) {
            *gsm_reg = 1;
        } else {
            *gsm_reg = status;
        }
    }

    if (act_str) {
        errno          = 0;
        const int lact = (int)strtol(act_str, (char**)NULL, 10);
        if (errno == EINVAL) {
            *act = -1;
        } else {
            *act = lact;
        }
    }

    return 0;
}

int at_parse_cmti(const char* str, int* idx)
{
    /*
     * Parse cmti info in the following format:
     *
     *   +CMTI: <mem>,<index>
     *
     * Example:
     *
     *   +CMTI: ,2
     *   +CMTI: ,-1
     */

    int lidx;
    const int res = sscanf(str, "+CMTI:%*[^,],%d", &lidx);
    if (res != 1) {
        return -1;
    }

    *idx = lidx;
    return 0;
}

int at_parse_cdsi(const char* str, int* idx)
{
    /*
     * parse cdsi info in the following format:
     *
     *   +CDSI: <mem>,<index>
     */

    int lidx;
    const int res = sscanf(str, "+CDSI:%*[^,],%d", &lidx);
    if (res != 1) {
        return -1;
    }

    *idx = lidx;
    return 0;
}

static int parse_pdu(const char* str, size_t len, int* tpdu_type, char* sca, size_t sca_len, char* oa, size_t oa_len, char* scts, int* mr, int* st, char* dt,
                     char* msg, size_t* msg_len, pdu_udh_t* udh)
{
    uint16_t msg16_tmp[256];

    int pdu_length = (unhex(str, (uint8_t*)str) + 1) / 2;
    if (pdu_length < 0) {
        chan_quectel_err = E_MALFORMED_HEXSTR;
        return -1;
    }

    int i   = 0;
    int res = pdu_parse_sca((uint8_t*)(str + i), pdu_length - i, sca, sca_len);
    if (res < 0) {
        /* tpdu_parse_sca sets chan_quectel_err */
        return -1;
    }
    i += res;
    if (len > (size_t)(pdu_length - i)) {
        chan_quectel_err = E_INVALID_TPDU_LENGTH;
        return -1;
    }
    res = tpdu_parse_type((uint8_t*)(str + i), pdu_length - i, tpdu_type);
    if (res < 0) {
        /* tpdu_parse_type sets chan_quectel_err */
        return -1;
    }
    i += res;
    switch (PDUTYPE_MTI(*tpdu_type)) {
        case PDUTYPE_MTI_SMS_STATUS_REPORT:
            res = tpdu_parse_status_report((uint8_t*)(str + i), pdu_length - i, mr, oa, oa_len, scts, dt, st);
            if (res < 0) {
                /* tpdu_parse_status_report sets chan_quectel_err */
                return -1;
            }
            break;

        case PDUTYPE_MTI_SMS_DELIVER:
            res = tpdu_parse_deliver((uint8_t*)(str + i), pdu_length - i, *tpdu_type, oa, oa_len, scts, msg16_tmp, udh);
            if (res < 0) {
                /* tpdu_parse_deliver sets chan_quectel_err */
                return -1;
            }
            res = ucs2_to_utf8(msg16_tmp, res, msg, *msg_len);
            if (res < 0) {
                chan_quectel_err = E_PARSE_UCS2;
                return -1;
            }
            *msg_len = res;
            msg[res] = '\0';
            break;

        default:
            chan_quectel_err = E_INVALID_TPDU_TYPE;
            return -1;
    }
    return 0;
}

/*!
 * \brief Parse a CMGR message
 * \param str -- pointer to pointer of string to parse (null terminated)
 * \param len -- string lenght
 * \param number -- a pointer to a char pointer which will store the from number
 * \param text -- a pointer to a char pointer which will store the message text
 * @note str will be modified when the CMGR message is parsed
 * \retval  0 success
 * \retval -1 parse error
 */

int at_parse_cmgr(char* str, size_t len, int* tpdu_type, char* sca, size_t sca_len, char* oa, size_t oa_len, char* scts, int* mr, int* st, char* dt, char* msg,
                  size_t* msg_len, pdu_udh_t* udh)
{
    /* skip "+CMGR:" */
    while (len > 0 && *str != ':') {
        ++str;
        --len;
    }
    if (len > 0 && *str == ':') {
        ++str;
        --len;
    }

    /* skip leading spaces */
    while (len > 0 && *str == ' ') {
        ++str;
        --len;
    }

    if (len <= 0) {
        chan_quectel_err = E_PARSE_CMGR_LINE;
        return -1;
    }
    if (str[0] == '"') {
        chan_quectel_err = E_DEPRECATED_CMGR_TEXT;
        return -1;
    }


    /*
     * parse cmgr info in the following PDU format
     * +CMGR: message_status,[address_text],TPDU_length<CR><LF>
     * SMSC_number_and_TPDU<CR><LF><CR><LF>
     * OK<CR><LF>
     *
     *	sample
     * +CMGR: 1,,31
     * 07911234567890F3040B911234556780F20008012150220040210C041F04400438043204350442<CR><LF><CR><LF>
     * OK<CR><LF>
     */

    static const char delimiters[] = ",,\n";

    char* marks[STRLEN(delimiters)];
    char* end;

    if (mark_line(str, delimiters, marks) != ARRAY_LEN(marks)) {
        chan_quectel_err = E_PARSE_CMGR_LINE;
        return -1;
    }

    const size_t tpdu_length = strtol(marks[1] + 1, &end, 10);
    if (tpdu_length <= 0 || end[0] != '\r') {
        chan_quectel_err = E_INVALID_TPDU_LENGTH;
        return -1;
    }

    return parse_pdu(marks[2] + 1, tpdu_length, tpdu_type, sca, sca_len, oa, oa_len, scts, mr, st, dt, msg, msg_len, udh);
}

int at_parse_cmt(char* str, size_t len, int* tpdu_type, char* sca, size_t sca_len, char* oa, size_t oa_len, char* scts, int* mr, int* st, char* dt, char* msg,
                 size_t* msg_len, pdu_udh_t* udh)
{
    /* skip "+CMT:" */
    str += 5;
    len -= 5;

    /* skip leading spaces */
    while (len > 0 && *str == ' ') {
        ++str;
        --len;
    }

    if (len <= 0) {
        chan_quectel_err = E_PARSE_CMGR_LINE;
        return -1;
    }


    /*
     * +CMT: [<alpha>],<length><CR><LF><pdu>
     */

    static const char delimiters[] = ",\n";

    char* marks[STRLEN(delimiters)];
    char* end;

    if (mark_line(str, delimiters, marks) != ARRAY_LEN(marks)) {
        chan_quectel_err = E_PARSE_CMGR_LINE;
        return -1;
    }

    const size_t tpdu_length = strtol(marks[0] + 1, &end, 10);
    if (tpdu_length <= 0 || end[0] != '\r') {
        chan_quectel_err = E_INVALID_TPDU_LENGTH;
        return -1;
    }

    return parse_pdu(marks[1] + 1, tpdu_length, tpdu_type, sca, sca_len, oa, oa_len, scts, mr, st, dt, msg, msg_len, udh);
}

int at_parse_cbm(char* str, size_t len, int* tpdu_type, char* sca, size_t sca_len, char* oa, size_t oa_len, char* scts, int* mr, int* st, char* dt, char* msg,
                 size_t* msg_len, pdu_udh_t* udh)
{
    /* skip "+CBM:" */
    str += 5;
    len -= 5;

    /* skip leading spaces */
    while (len > 0 && *str == ' ') {
        ++str;
        --len;
    }

    if (len <= 0) {
        chan_quectel_err = E_PARSE_CMGR_LINE;
        return -1;
    }
    if (str[0] == '"') {
        chan_quectel_err = E_DEPRECATED_CMGR_TEXT;
        return -1;
    }


    /*
     * +CBM: <length><CR><LF><pdu>
     */

    static const char delimiters[] = "\n";

    char* marks[STRLEN(delimiters)];
    char* end;

    if (mark_line(str, delimiters, marks) != ARRAY_LEN(marks)) {
        chan_quectel_err = E_PARSE_CMGR_LINE;
        return -1;
    }

    const size_t tpdu_length = strtol(str, &end, 10);
    if (tpdu_length <= 0 || end[0] != '\r') {
        chan_quectel_err = E_INVALID_TPDU_LENGTH;
        return -1;
    }

    return parse_pdu(marks[0] + 1, tpdu_length, tpdu_type, sca, sca_len, oa, oa_len, scts, mr, st, dt, msg, msg_len, udh);
}

int at_parse_cds(char* str, size_t len, int* tpdu_type, char* sca, size_t sca_len, char* oa, size_t oa_len, char* scts, int* mr, int* st, char* dt, char* msg,
                 size_t* msg_len, pdu_udh_t* udh)
{
    /* skip "+CDS:" */
    str += 5;
    len -= 5;

    /* skip leading spaces */
    while (len > 0 && *str == ' ') {
        ++str;
        --len;
    }

    if (len <= 0) {
        chan_quectel_err = E_PARSE_CMGR_LINE;
        return -1;
    }
    if (str[0] == '"') {
        chan_quectel_err = E_DEPRECATED_CMGR_TEXT;
        return -1;
    }


    /*
     * +CDS: <length><CR><LF><pdu>
     */

    static const char delimiters[] = "\n";

    char* marks[STRLEN(delimiters)];
    char* end;

    if (mark_line(str, delimiters, marks) != ARRAY_LEN(marks)) {
        chan_quectel_err = E_PARSE_CMGR_LINE;
        return -1;
    }

    const size_t tpdu_length = strtol(str, &end, 10);
    if (tpdu_length <= 0 || end[0] != '\r') {
        chan_quectel_err = E_INVALID_TPDU_LENGTH;
        return -1;
    }

    return parse_pdu(marks[0] + 1, tpdu_length, tpdu_type, sca, sca_len, oa, oa_len, scts, mr, st, dt, msg, msg_len, udh);
}

int at_parse_cmgl(char* str, size_t len, int* idx, int* tpdu_type, char* sca, size_t sca_len, char* oa, size_t oa_len, char* scts, int* mr, int* st, char* dt,
                  char* msg, size_t* msg_len, pdu_udh_t* udh)
{
    /* skip "+CMGL:" */
    str += 6;
    len -= 6;

    /* skip leading spaces */
    while (len > 0 && *str == ' ') {
        ++str;
        --len;
    }

    if (len <= 0) {
        chan_quectel_err = E_PARSE_CMGR_LINE;
        return -1;
    }
    if (str[0] == '"') {
        chan_quectel_err = E_DEPRECATED_CMGR_TEXT;
        return -1;
    }


    /*
     * parse cmgr info in the following PDU format
     * +CMGL: idx,message_status,[address_text],TPDU_length<CR><LF>
     * SMSC_number_and_TPDU<CR><LF><CR><LF>
     * OK<CR><LF>
     *
     *	Exanoke
     * +CMGL: 0,1,,31
     * 07911234567890F3040B911234556780F20008012150220040210C041F04400438043204350442<CR><LF><CR><LF>
     * OK<CR><LF>
     */

    static const char delimiters[] = ",,,\n";

    char* marks[STRLEN(delimiters)];
    char* end;

    if (mark_line(str, delimiters, marks) != ARRAY_LEN(marks)) {
        chan_quectel_err = E_PARSE_CMGR_LINE;
        return -1;
    }

    *idx = strtol(str, &end, 10);
    if (*idx < 0 || end[0] != ',') {
        chan_quectel_err = E_UNKNOWN;
        return -1;
    }

    const size_t tpdu_length = strtol(marks[2] + 1, &end, 10);
    if (tpdu_length <= 0 || end[0] != '\r') {
        chan_quectel_err = E_INVALID_TPDU_LENGTH;
        return -1;
    }

    return parse_pdu(marks[3] + 1, tpdu_length, tpdu_type, sca, sca_len, oa, oa_len, scts, mr, st, dt, msg, msg_len, udh);
}

/*!
 * \brief Parse a +CMGS notification
 * \param str -- string to parse (null terminated)
 * \return -1 on error (parse error) or the first integer value found
 * \todo FIXME: parse <mr>[,<scts>] value correctly
 */

int at_parse_cmgs(const char* str)
{
    int cmgs = -1;

    /*
     * parse CMGS info in the following format:
     * +CMGS:<mr>[,<scts>]
     */
    return sscanf(str, "+CMGS:%d", &cmgs) ? cmgs : -1;
}

/*!
 * \brief Parse a CUSD answer
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * @note str will be modified when the CUSD string is parsed
 * \retval  0 success
 * \retval -1 parse error
 */

int at_parse_cusd(char* str, int* type, char** cusd, int* dcs)
{
    /*
     * parse cusd message in the following format:
     * +CUSD: <m>,[<str>,<dcs>]
     *
     * examples
     *   +CUSD: 5
     *   +CUSD: 0,"100,00 EURO, valid till 01.01.2010, you are using tariff "Mega Tariff". More informations *111#.",15
     */

    static char delimiters[] = ":,,";
    char* marks[STRLEN(delimiters)];

    const unsigned count = mark_line(str, delimiters, marks);

    if (!count) {
        return -1;
    }

    trim_line(marks, count);
    if (sscanf(marks[0] + 1, "%u", type) != 1) {
        return -1;
    }

    if (count > 1) {
        *cusd = strip_quoted(marks[1] + 1);
        if (count > 2) {
            const char* const dcs_str = ast_skip_blanks(marks[2] + 1);
            if (!*dcs_str) {
                dcs = 0;
            } else if (sscanf(dcs_str, "%u", dcs) != 1) {
                return -1;
            }
        } else {
            *dcs = -1;
        }
    } else {
        *cusd = "";
        *dcs  = -1;
    }

    return 0;
}

/*!
 * \brief Parse a CPIN notification
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \return  2 if PUK required
 * \return  1 if PIN required
 * \return  0 if no PIN required
 * \return -1 on error (parse error) or card lock
 */

int at_parse_cpin(const char* str, const size_t len)
{
    static const struct {
        const char* value;
        unsigned length;
    } resp[] = {
        {"READY",   5},
        {"SIM PIN", 7},
        {"SIM PUK", 7},
    };

    for (unsigned int idx = 0; idx < ARRAY_LEN(resp); ++idx) {
        if (memmem(str, len, resp[idx].value, resp[idx].length)) {
            return (int)idx;
        }
    }
    return -1;
}

/*!
 * \brief Parse +CSQ response
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int at_parse_csq(const char* str, int* rssi)
{
    /*
     * parse +CSQ response in the following format:
     * +CSQ: <RSSI>,<BER>
     */

    return sscanf(str, "+CSQ:%2d,", rssi) == 1 ? 0 : -1;
}

int at_parse_csqn(char* str, int* rssi, int* ber)
{
    /*
        Example:

        +CSQN: 25,0
    */

    char* t = ast_strsep(&str, ':', 0);
    if (!t) {
        return -1;
    }
    t = ast_strsep(&str, ':', 0);
    if (!t) {
        return -1;
    }
    t             = ast_skip_blanks(t);
    const int res = sscanf(t, "%d,%d", rssi, ber);
    return (res == 2) ? 0 : -1;
}

/*!
 * \brief Parse a ^RSSI notification
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \return -1 on error (parse error) or the rssi value
 */

int at_parse_rssi(const char* str)
{
    int rssi = -1;

    /*
     * parse RSSI info in the following format:
     * ^RSSI:<rssi>
     */

    sscanf(str, "^RSSI:%d", &rssi);
    return rssi;
}

int at_parse_qind(char* str, qind_t* qind, char** params)
{
    static const char delimiters[] = "\"\",";
    char* marks[STRLEN(delimiters)];

    if (mark_line(str, delimiters, marks) == ARRAY_LEN(marks)) {
        const char* qind_str = marks[0] + 1;
        marks[1][0]          = '\000';

        if (!strcmp(qind_str, "csq")) {
            *qind = QIND_CSQ;
        } else if (!strcmp(qind_str, "act")) {
            *qind = QIND_ACT;
        } else if (!strcmp(qind_str, "ccinfo")) {
            *qind = QIND_CCINFO;
        } else {
            *qind = QIND_NONE;
        }

        *params = marks[2] + 1;
        return 0;
    }

    return -1;
}

int at_parse_qind_csq(const char* params, int* rssi)
{
    /*
     * parse notification in the following format:
     * +QIND: "csq",<RSSI>,<BER>
     */

    return sscanf(params, "%d", rssi) == 1 ? 0 : -1;
}

int at_parse_qind_act(char* params, int* act)
{
    /*
     * parse notification in the following format:
     * +QIND: "act","<val>"
     */

    *act = act2int(strip_quoted(params));
    return 0;
}

int at_parse_qind_cc(char* params, unsigned* call_idx, unsigned* dir, unsigned* state, unsigned* mode, unsigned* mpty, char** number, unsigned* toa)
{
    /*
     * +QIND: "ccinfo",<idx>,<dir>,<state>,<mode>,<mpty>,<number>,<type>[,<alpha>]
     *
     * examples
     *  +QIND: "ccinfo",2,0,3,0,0,"XXXXXXXXX",129
     *  +QIND: "ccinfo",2,0,-1,0,0,"XXXXXXXXX",129 [-1 => 7]
     */
    static const char delimiters[] = ",,,,,,,";
    static const size_t nmarks     = STRLEN(delimiters) - 1u;

    char* marks[STRLEN(delimiters)];

    if (mark_line(params, delimiters, marks) < nmarks) {
        return -1;
    }

    int cc_state;
    if (sscanf(params, "%u", call_idx) == 1 && sscanf(marks[0] + 1, "%u", dir) == 1 && sscanf(marks[1] + 1, "%d", &cc_state) == 1 &&
        sscanf(marks[2] + 1, "%u", mode) == 1 && sscanf(marks[3] + 1, "%u", mpty) == 1 && sscanf(marks[5] + 1, "%u", toa) == 1) {
        marks[4]++;
        if (marks[4][0] == '"') {
            marks[4]++;
        }
        if (marks[5][-1] == '"') {
            marks[5]--;
        }
        *number     = marks[4];
        marks[5][0] = '\000';

        *state = (cc_state < 0) ? CALL_STATE_RELEASED : (unsigned)cc_state;
        return 0;
    }

    return -1;
}

#/* */

int at_parse_csca(char* str, char** csca)
{
    /**
     * Parse CSCA info in the following format:
     *
     * +CSCA: <SCA>,<TOSCA>
     * +CSCA: "+79139131234",145
     * +CSCA: "",145
     */

    static char delimiters[]       = ":,";
    static unsigned delimiters_cnt = STRLEN(delimiters) - 1u;

    char* marks[STRLEN(delimiters)];
    const unsigned markscnt = mark_line(str, delimiters, marks);

    if (markscnt < delimiters_cnt) {
        return -1;
    }

    trim_line(marks, markscnt);
    *csca = strip_quoted(marks[0] + 1);
    return 0;
}

#/* */

int at_parse_dsci(char* str, unsigned* call_idx, unsigned* dir, unsigned* state, unsigned* call_type, char** number, unsigned* toa)
{
    /*
     * ^DSCI: <id>,<dir>,<stat>,<type>,<number>,<num_type>[,<tone_info>]\r\n

     * examples
     *
     * ^DSCI: 2,1,4,0,+48XXXXXXXXX,145
     * ^DSCI: 2,1,6,0,+48XXXXXXXXX,145
     */

    static const char delimiters[] = ":,,,,,,";
    static const size_t nmarks     = STRLEN(delimiters) - 1u;

    char* marks[STRLEN(delimiters)];

    if (mark_line(str, delimiters, marks) >= nmarks) {
        if (sscanf(marks[0] + 1, "%u", call_idx) == 1 && sscanf(marks[1] + 1, "%u", dir) == 1 && sscanf(marks[2] + 1, "%u", state) == 1 &&
            sscanf(marks[3] + 1, "%u", call_type) == 1 && sscanf(marks[5] + 1, "%u", toa) == 1) {
            *number     = marks[4] + 1;
            marks[5][0] = '\000';

            return 0;
        }
    }

    return -1;
}

#/* */

int at_parse_clcc(char* str, unsigned* call_idx, unsigned* dir, unsigned* state, unsigned* mode, unsigned* mpty, char** number, unsigned* toa)
{
    /*
     * +CLCC:<id1>,<dir>,<stat>,<mode>,<mpty>[,<number>,<type>[,<alpha>[,<priority>]]]\r\n
     *  ...
     * +CLCC:<id1>,<dir>,<stat>,<mode>,<mpty>[,<number>,<type>[,<alpha>[,<priority>]]]\r\n
     *  examples
     *   +CLCC: 1,1,4,0,0,"",145
     *   +CLCC: 1,1,4,0,0,"+79139131234",145
     *   +CLCC: 1,1,4,0,0,"0079139131234",145
     *   +CLCC: 1,1,4,0,0,"+7913913ABCA",145
     */
    static const char delimiters[] = ":,,,,,,";
    char* marks[STRLEN(delimiters)];

    if (mark_line(str, delimiters, marks) == ARRAY_LEN(marks)) {
        if (sscanf(marks[0] + 1, "%u", call_idx) == 1 && sscanf(marks[1] + 1, "%u", dir) == 1 && sscanf(marks[2] + 1, "%u", state) == 1 &&
            sscanf(marks[3] + 1, "%u", mode) == 1 && sscanf(marks[4] + 1, "%u", mpty) == 1 && sscanf(marks[6] + 1, "%u", toa) == 1) {
            marks[5]++;
            if (marks[5][0] == '"') {
                marks[5]++;
            }
            if (marks[6][-1] == '"') {
                marks[6]--;
            }
            *number     = marks[5];
            marks[6][0] = '\000';

            return 0;
        }
    }

    return -1;
}

#/* */

int at_parse_ccwa(char* str, ccwa_variant_t* variant, unsigned int* status /* or type */, unsigned int* class)
{
    /*
     * CCWA may be in form:
     *	in response of AT+CCWA=?
     *		+CCWA: (0,1)
     *	in response of AT+CCWA=?
     *		+CCWA: <n>
     *	in response of "AT+CCWA=[<n>[,<mode>[,<class>]]]"
     *		+CCWA: <status>,<class1>
     *
     *	unsolicited result code
     *		+CCWA: <number>,<type>,<class>,[<alpha>][,<CLI validity>[,<subaddr>,<satype>[,<priority>]]]
     */
    static char delimiters[] = ":,,";
    char* marks[STRLEN(delimiters)];

    /* parse URC only here */
    const int res = mark_line(str, delimiters, marks);
    switch (res) {
        case ARRAY_LEN(marks):
            trim_line(marks, res);
            if (!sscanf(marks[1] + 1, "%u", status) || !sscanf(marks[2] + 1, "%u", class)) {
                return -1;
            }
            *variant = CCWA_VARIANT_URC;
            break;

        case ARRAY_LEN(marks) - 1u:
            trim_line(marks, res);
            if (!sscanf(marks[0] + 1, "%u", status) || !sscanf(marks[1] + 1, "%u", class)) {
                return -1;
            }
            *variant = CCWA_VARIANT_STATUS_AND_CLASS;
            break;

        case ARRAY_LEN(marks) - 2u:
            trim_line(marks, res);
            if (!sscanf(marks[0] + 1, "%u", status)) {
                return -1;
            }
            *variant = CCWA_VARIANT_PRESENTATION_FLAG;
            break;
    }

    return 0;
}

int at_parse_qtonedet(const char* str, int* dtmf)
{
    /*
        Example:

        +QTONEDET: 56
    */

    return sscanf(str, "+QTONEDET:%d,", dtmf) == 1 ? 0 : -1;
}

int at_parse_dtmf(char* str, char* dtmf)
{
    /*
        Example:

        +RXDTMF: 5
        +DTMF:5
    */

    char* t = ast_strsep(&str, ':', 0);
    if (!t) {
        return -1;
    }
    t = ast_strsep(&str, ':', AST_STRSEP_TRIM);
    if (!t) {
        return -1;
    }
    *dtmf = *t;
    return 0;
}

int at_parse_qpcmv(char* str, int* enabled, int* mode)
{
    return 0;
}

int at_parse_qlts(char* str, char** ts)
{
    /*
        +QLTS: "2017/10/13,03:40:48+32,0"
    */

    static const char delimiters[] = ":";
    char* marks[STRLEN(delimiters)];

    if (mark_line(str, delimiters, marks) == 1) {
        *ts = strip_quoted(marks[0] + 1);
        return 0;
    }

    return -1;
}

int at_parse_cclk(char* str, char** ts)
{
    /*
        +CCLK: “08/11/26,10:15:02+32”
    */

    static const char delimiters[] = ":";
    char* marks[STRLEN(delimiters)];

    if (mark_line(str, delimiters, marks) == 1) {
        *ts = strip_quoted(marks[0] + 1);
        return 0;
    }

    return -1;
}

int at_parse_qrxgain(const char* str, int* gain)
{
    /*
        Example:

        +QRXGAIN: 20577
    */

    return sscanf(str, "+QRXGAIN:%d,", gain) == 1 ? 0 : -1;
}

int at_parse_qmic(const char* str, int* gain, int* dgain)
{
    /*
        Example:

        +QMIC: 20577,16001
    */

    return sscanf(str, "+QMIC:%d,%d", gain, dgain) == 2 ? 0 : -1;
}

int at_parse_cxxxgain(const char* str, int* gain)
{
    /*
        Example:

        +CMICGAIN: 3
        +COUTGAIN: 8
    */

    static const char CXXVOL[] = "+CXXXGAIN:";

    const unsigned int g = (unsigned int)strtoul(str + STRLEN(CXXVOL), NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    *gain = g;
    return 0;
}

int at_parse_cxxvol(const char* str, int* gain)
{
    /*
        Example:

        +CTXVOL: 0x1234
        +CRXVOL: 0x1234
    */

    static const char CXXVOL[] = "+CXXVOL:";

    const unsigned int g = (unsigned int)strtoul(str + STRLEN(CXXVOL), NULL, 0);
    if (errno == ERANGE) {
        return -1;
    }

    *gain = g;
    return 0;
}

int at_parse_qaudloop(const char* str, int* aloop)
{
    /*
        Example:

        +QAUDLOOP: 0
    */

    return sscanf(str, "+QAUDLOOP:%d,", aloop) == 1 ? 0 : -1;
}

int at_parse_qaudmod(const char* str, int* amode)
{
    /*
        Example:

        +QAUDMOD: 3
    */

    return sscanf(str, "+QAUDMOD:%d,", amode) == 1 ? 0 : -1;
}

int at_parse_cgmr(const char* str, char** cgmr)
{
    static const char CGMR[] = "+CGMR: ";
    *cgmr                    = ast_skip_blanks(str + STRLEN(CGMR));
    return 0;
}

int at_parse_cpcmreg(const char* str, int* pcmreg)
{
    /*
        Example:

        +CPCMREG: 0
    */

    return sscanf(str, "+CPCMREG:%d,", pcmreg) == 1 ? 0 : -1;
}

int at_parse_cnsmod(char* str, int* act)
{
    /*
        Example:

        +CNSMOD: 8
        +CNSMOD: 1,7
    */

    const char* act_str = NULL;

    const char* t = ast_strsep(&str, ':', 0);
    if (!t) {
        return -1;
    }
    t = ast_strsep(&str, ',', AST_STRSEP_TRIM);
    if (!t) {
        return -1;
    }
    act_str = t;
    t       = ast_strsep(&str, ',', AST_STRSEP_TRIM);
    if (t) {
        act_str = t;
    }

    if (!act_str) {
        return -1;
    }
    return sscanf(act_str, "%d", act) == 1 ? 0 : -1;
}

int at_parse_cring(char* str, char** ring_type)
{
    /*
        Example:

        +CRING: VOICE
    */

    char* t = ast_strsep(&str, ':', 0);
    if (!t) {
        return -1;
    }
    t = ast_strsep(&str, ':', AST_STRSEP_STRIP);
    if (!t) {
        return -1;
    }

    *ring_type = t;
    return 0;
}

int at_parse_psnwid(char* str, int* mcc, int* mnc, char** fnn, char** snn)
{
    /*
        *PSNWID: "<mcc>", "<mnc>", "<full network name>",<full network name CI>, "<short network name>",<short network
       name CI>

        *PSNWID: "260","03", "004F00720061006E00670065", 0, "004F00720061006E00670065", 0
    */

    static char delimiters[] = ":,,,,,";
    char* marks[STRLEN(delimiters)];

    if (mark_line(str, delimiters, marks) != ARRAY_LEN(marks)) {
        return -1;
    }

    marks[1][0] = marks[2][0] = marks[3][0] = marks[4][0] = marks[5][0] = '\000';

    char* mcc_str = ast_skip_blanks(marks[0] + 1);
    mcc_str       = strip_quoted(mcc_str);

    char* mnc_str = ast_skip_blanks(marks[1] + 1);
    mnc_str       = strip_quoted(mnc_str);

    char* fnn_str = ast_skip_blanks(marks[2] + 1);
    fnn_str       = strip_quoted(fnn_str);

    char* snn_str = ast_skip_blanks(marks[4] + 1);
    snn_str       = strip_quoted(snn_str);

    *mcc = (int)strtoul(mcc_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    *mnc = (int)strtoul(mnc_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    *fnn = fnn_str;
    *snn = snn_str;

    return 0;
}

int at_parse_ciev_10(char* str, int* plmn, char** fnn, char** snn)
{
    /*
        SimCOM800 specific - not documented.

        Examples:

        +CIEV: 10,"26003","Orange","Orange", 0, 0
    */

    static char delimiters[] = ":,,,,,";
    char* marks[STRLEN(delimiters)];

    if (mark_line(str, delimiters, marks) != ARRAY_LEN(marks)) {
        return -1;
    }

    trim_line(marks, ARRAY_LEN(marks));

    char* ind_str = ast_skip_blanks(marks[0] + 1);
    ind_str       = strip_quoted(ind_str);

    char* plmn_str = ast_skip_blanks(marks[1] + 1);
    plmn_str       = strip_quoted(plmn_str);

    char* fnn_str = ast_skip_blanks(marks[2] + 1);
    fnn_str       = strip_quoted(fnn_str);

    char* snn_str = ast_skip_blanks(marks[3] + 1);
    snn_str       = strip_quoted(snn_str);

    const long ind = strtoul(ind_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    if (ind != 10) {  // the only known indicator
        return -1;
    }

    *plmn = (int)strtoul(plmn_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    *fnn = fnn_str;
    *snn = snn_str;
    return 0;
}

int at_parse_psuttz(char* str, int* year, int* month, int* day, int* hour, int* min, int* sec, int* tz, int* dst)
{
    /*
        *PSUTTZ: <year>,<month>,<day>,<hour>,<min>,<sec>, "<time zone>",<dst>

        *PSUTTZ: 2023, 6, 8, 5, 15, 27, "+8", 1
    */

    static char delimiters[] = ":,,,,,,,";
    char* marks[STRLEN(delimiters)];

    if (mark_line(str, delimiters, marks) != ARRAY_LEN(marks)) {
        return -1;
    }

    marks[1][0] = marks[2][0] = marks[3][0] = marks[4][0] = marks[5][0] = marks[6][0] = marks[7][0] = '\000';

    char* year_str  = ast_skip_blanks(marks[0] + 1);
    char* month_str = ast_skip_blanks(marks[1] + 1);
    char* day_str   = ast_skip_blanks(marks[2] + 1);
    char* hour_str  = ast_skip_blanks(marks[3] + 1);
    char* min_str   = ast_skip_blanks(marks[4] + 1);
    char* sec_str   = ast_skip_blanks(marks[5] + 1);
    char* tz_str    = ast_skip_blanks(marks[6] + 1);
    tz_str          = strip_quoted(tz_str);
    if (*tz_str == '+') {
        tz_str += 1;
    }
    char* dst_str = ast_skip_blanks(marks[7] + 1);

    *year = (int)strtoul(year_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    *month = (int)strtoul(month_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    *day = (int)strtoul(day_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    *hour = (int)strtoul(hour_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    *min = (int)strtoul(min_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    *sec = (int)strtoul(sec_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    *tz = (int)strtoul(tz_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    *dst = (int)strtoul(dst_str, NULL, 10);
    if (errno == ERANGE) {
        return -1;
    }

    return 0;
}

int at_parse_revision(char* str, char** rev)
{
    char* t = ast_strsep(&str, ':', 0);
    if (!t) {
        return -1;
    }
    t = ast_strsep(&str, ':', AST_STRSEP_STRIP);
    if (!t) {
        return -1;
    }
    *rev = t;
    return 0;
}

int at_parse_xccid(char* str, char** ccid)
{
    char* t = ast_strsep(&str, ':', 0);
    if (!t) {
        return -1;
    }
    t = ast_strsep(&str, ':', AST_STRSEP_STRIP);
    if (!t) {
        return -1;
    }
    *ccid = t;
    return 0;
}
