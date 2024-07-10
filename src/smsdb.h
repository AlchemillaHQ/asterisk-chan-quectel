/*
   Copyright (C) 2020 Max von Buelow <max@m9x.de>
*/

#ifndef CHAN_QUECTEL_SMSDB_H_INCLUDED
#define CHAN_QUECTEL_SMSDB_H_INCLUDED

int smsdb_init();
void smsdb_atexit();
int smsdb_put(const char* id, const char* addr, int ref, int parts, int order, const char* msg, struct ast_str** out);
int smsdb_get_refid(const char* id, const char* addr);
int smsdb_outgoing_add(const char* id, const char* addr, const char* msg, int cnt, int ttl, int srr);
int smsdb_outgoing_clear(int uid, struct ast_str** dst, struct ast_str** msg);
int smsdb_outgoing_part_put(int uid, int refid, struct ast_str** dst, struct ast_str** msg);
int smsdb_outgoing_part_status(const char* id, const char* addr, int mr, int st, int* status_all);
int smsdb_outgoing_purge_one(int* uid, struct ast_str** dst, struct ast_str** msg);
int smsdb_vacuum_into(const char* backup_file);

#endif
