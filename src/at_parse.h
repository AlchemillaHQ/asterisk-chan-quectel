/*
    Copyright (C) 2010 bg <bg_one@mail.ru>
    Copyright (C) 2020 Max von Buelow <max@m9x.de>
*/
#ifndef CHAN_QUECTEL_AT_PARSE_H_INCLUDED
#define CHAN_QUECTEL_AT_PARSE_H_INCLUDED

#include <sys/types.h> /* size_t */

#include "char_conv.h" /* str_encoding_t */
#include "pdu.h"

struct pvt;

typedef enum { QIND_NONE = 0, QIND_CSQ, QIND_ACT, QIND_CCINFO } qind_t;

typedef enum { CCWA_VARIANT_PRESENTATION_FLAG, CCWA_VARIANT_STATUS_AND_CLASS, CCWA_VARIANT_URC } ccwa_variant_t;

const char* at_qind2str(qind_t);

char* at_parse_cnum(char* str);
int at_parse_cops(char* str, char** oper, int* act);
int at_parse_qspn(char* str, char** fnn, char** snn, char** spn);
int at_parse_cspn(char*, char**);
int at_parse_qnwinfo(char* str, int* act, int* oper, char** band, int* channel);
int at_parse_creg(char* str, int cereg, int* gsm_reg_status, char** lac, char** ci, int* act);
int at_parse_cmti(const char* str, int* idx);
int at_parse_cdsi(const char* str, int* idx);
int at_parse_cmgr(char* str, size_t len, int* tpdu_type, char* sca, size_t sca_len, char* oa, size_t oa_len, struct ast_tm* scts, int* mr, int* st,
                  struct ast_tm* dt, char* msg, size_t* msg_len, pdu_udh_t* udh);
int at_parse_cmgl(char* str, size_t len, int* idx, int* tpdu_type, char* sca, size_t sca_len, char* oa, size_t oa_len, struct ast_tm* scts, int* mr, int* st,
                  struct ast_tm* dt, char* msg, size_t* msg_len, pdu_udh_t* udh);
int at_parse_cmt(char* str, size_t len, int* tpdu_type, char* sca, size_t sca_len, char* oa, size_t oa_len, struct ast_tm* scts, int* mr, int* st,
                 struct ast_tm* dt, char* msg, size_t* msg_len, pdu_udh_t* udh);
int at_parse_cbm(char* str, size_t len, int* tpdu_type, char* sca, size_t sca_len, char* oa, size_t oa_len, struct ast_tm* scts, int* mr, int* st,
                 struct ast_tm* dt, char* msg, size_t* msg_len, pdu_udh_t* udh);
int at_parse_cds(char* str, size_t len, int* tpdu_type, char* sca, size_t sca_len, char* oa, size_t oa_len, struct ast_tm* scts, int* mr, int* st,
                 struct ast_tm* dt, char* msg, size_t* msg_len, pdu_udh_t* udh);
int at_parse_cmgs(const char* str);
int at_parse_cusd(char* str, int* type, char** cusd, int* dcs);
int at_parse_cpin(const char* str, const size_t len);
int at_parse_csq(const char* str, int* rssi);
int at_parse_csqn(char*, int*, int*);
int at_parse_rssi(const char* str);
int at_parse_qind(char* str, qind_t* qind, char** params);
int at_parse_qind_csq(const char* params, int* rssi);
int at_parse_qind_act(char* params, int* act);
int at_parse_qind_cc(char* params, unsigned* call_idx, unsigned* dir, unsigned* state, unsigned* mode, unsigned* mpty, char** number, unsigned* toa);
int at_parse_csca(char* str, char** csca);
int at_parse_dsci(char* str, unsigned* call_idx, unsigned* dir, unsigned* state, unsigned* call_type, char** number, unsigned* toa);
int at_parse_clcc(char* str, unsigned* call_idx, unsigned* dir, unsigned* state, unsigned* mode, unsigned* mpty, char** number, unsigned* toa);
int at_parse_ccwa(char* str, ccwa_variant_t* variant, unsigned int*, unsigned int* class);
int at_parse_qtonedet(const char* str, int* dtmf);
int at_parse_dtmf(char* str, char* dtmf);
int at_parse_qpcmv(char* str, int* enabled, int* mode);
int at_parse_qlts(char* str, struct ast_tm* const ts);
int at_parse_cclk(char* str, struct ast_tm* ts);
int at_parse_qrxgain(const char* str, int* gain);
int at_parse_qmic(const char* str, int* gain, int* dgain);
int at_parse_cxxxgain(const char* str, int* gain);
int at_parse_cxxvol(const char*, int*);
int at_parse_qaudloop(const char*, int*);
int at_parse_qaudmod(const char*, int*);
int at_parse_cgmr(const char*, char**);
int at_parse_cpcmreg(const char*, int*);
int at_parse_cnsmod(char*, int*);
int at_parse_cring(char*, char**);
int at_parse_psnwid(char*, int*, int*, char**, char**);
int at_parse_ciev_10(char*, int*, char**, char**);
int at_parse_psuttz(char*, int*, int*, int*, int*, int*, int*, int*, int*);
int at_parse_revision(char*, char**);
int at_parse_xccid(char*, char**);

#endif /* CHAN_QUECTEL_AT_PARSE_H_INCLUDED */
