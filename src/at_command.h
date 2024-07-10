/*
   Copyright (C) 2010 bg <bg_one@mail.ru>
*/
#ifndef CHAN_QUECTEL_AT_SEND_H_INCLUDED
#define CHAN_QUECTEL_AT_SEND_H_INCLUDED

#include "ast_config.h"

#include "dc_config.h" /* call_waiting_t */
#include "mutils.h"    /* enum2str_def() ITEMS_OF() */

#define CCWA_CLASS_VOICE 1
#define SMS_INDEX_MAX 256 /* exclusive */

/* AT_COMMANDS_TABLE */
#define AT_CMD_AS_ENUM(cmd, str) CMD_##cmd,
#define AT_CMD_AS_STRING(cmd, str) str,

#define AT_COMMANDS_TABLE(_)                        \
    _(USER, "USER")                                 \
    _(AT, "AT")                                     \
    _(AT_FINAL, "AT")                               \
    _(AT_A, "ATA")                                  \
    _(AT_CCWA_STATUS, "AT+CCWA?")                   \
    _(AT_CCWA_SET, "AT+CCWA=")                      \
    _(AT_CFUN, "AT+CFUN")                           \
                                                    \
    _(AT_CGMI, "AT+CGMI")                           \
    _(AT_CGMM, "AT+CGMM")                           \
    _(AT_CGMR, "AT+CGMR")                           \
    _(AT_CGSN, "AT+CGSN")                           \
                                                    \
    _(AT_CHUP, "AT+CHUP")                           \
    _(AT_QHUP, "AT+QHUP")                           \
    _(AT_CIMI, "AT+CIMI")                           \
    _(AT_CPCMREG, "AT+CPCMREG?")                    \
    _(AT_CLIR, "AT+CLIR")                           \
                                                    \
    _(AT_CLVL, "AT+CLVL")                           \
    _(AT_CMGD, "AT+CMGD")                           \
    _(AT_CMGF, "AT+CMGF")                           \
    _(AT_CMGR, "AT+CMGR")                           \
    _(AT_CMGL, "AT+CMGL")                           \
                                                    \
    _(AT_CMGS, "AT+CMGS")                           \
    _(AT_SMSTEXT, "SMSTEXT")                        \
    _(AT_CNMI, "AT+CNMI")                           \
    _(AT_CNUM, "AT+CNUM")                           \
                                                    \
    _(AT_COPS, "AT+COPS?")                          \
    _(AT_QSPN, "AT+QSPN")                           \
    _(AT_CSPN, "AT+CSPN")                           \
    _(AT_COPS_INIT, "AT+COPS=")                     \
    _(AT_CPIN, "AT+CPIN?")                          \
    _(AT_CPMS, "AT+CPMS")                           \
                                                    \
    _(AT_CREG, "AT+CREG?")                          \
    _(AT_CREG_INIT, "AT+CREG=")                     \
    _(AT_CEREG, "AT+CEREG?")                        \
    _(AT_CEREG_INIT, "AT+CEREG=")                   \
    _(AT_CSCS, "AT+CSCS")                           \
    _(AT_CSQ, "AT+CSQ")                             \
    _(AT_AUTOCSQ_INIT, "AT+AUTOCSQ=")               \
    _(AT_EXUNSOL_INIT, "AT+EXUNSOL=")               \
    _(AT_CLTS_INIT, "AT+CLTS=")                     \
                                                    \
    _(AT_CSSN, "AT+CSSN")                           \
    _(AT_CUSD, "AT+CUSD")                           \
    _(AT_CVOICE, "AT+QPCMV?")                       \
    _(AT_D, "ATD")                                  \
                                                    \
    _(AT_CPCMREG1, "AT+CPCMREG=1")                  \
    _(AT_CPCMREG0, "AT+CPCMREG=0")                  \
    _(AT_DTMF, "AT+VTS")                            \
    _(AT_E, "ATE")                                  \
                                                    \
    _(AT_Z, "ATZ")                                  \
    _(AT_CMEE, "AT+CMEE")                           \
    _(AT_CSCA, "AT+CSCA")                           \
                                                    \
    _(AT_CHLD_1x, "AT+CHLD=1x")                     \
    _(AT_CHLD_2x, "AT+CHLD=2x")                     \
    _(AT_CHLD_2, "AT+CHLD=2")                       \
    _(AT_CHLD_3, "AT+CHLD=3")                       \
    _(AT_CLCC, "AT+CLCC")                           \
    _(AT_QINDCFG_CSQ, "AT+QINDCFG=\"csq\"")         \
    _(AT_QINDCFG_ACT, "AT+QINDCFG=\"act\"")         \
    _(AT_QINDCFG_RING, "AT+QINDCFG=\"ring\"")       \
    _(AT_QINDCFG_CC, "AT+QINDCFG=\"ccinfo\",1")     \
    _(AT_QINDCFG_CC_OFF, "AT+QINDCFG=\"ccinfo\",0") \
    _(AT_DSCI, "AT^DSCI=1")                         \
    _(AT_DSCI_OFF, "AT^DSCI=0")                     \
    _(AT_CMUT_0, "AT+CMUT=0")                       \
    _(AT_CMUT_1, "AT+CMUT=1")                       \
    _(AT_QPCMV_0, "AT+QPCMV=0")                     \
    _(AT_QPCMV_TTY, "AT+QPCMV=1,0")                 \
    _(AT_QPCMV_UAC, "AT+QPCMV=1,2")                 \
    _(AT_QTONEDET_0, "AT+QTONEDET=0")               \
    _(AT_QTONEDET_1, "AT+QTONEDET=1")               \
    _(AT_DDET_0, "AT+DDET=0")                       \
    _(AT_DDET_1, "AT+DDET=1")                       \
    _(AT_QLTS, "AT+QLTS")                           \
    _(AT_QLTS_1, "AT+QLTS=1")                       \
    _(AT_CCLK, "AT+CCLK")                           \
    _(AT_QMIC, "AT+QMIC")                           \
    _(AT_QRXGAIN, "AT+QRXGAIN")                     \
    _(AT_CMICGAIN, "AT+CMICGAIN")                   \
    _(AT_COUTGAIN, "AT+COUTGAIN")                   \
    _(AT_CTXVOL, "AT+CTXVOL")                       \
    _(AT_CRXVOL, "AT+CRXVOL")                       \
    _(AT_CNMA, "AT+CNMA")                           \
    _(AT_CSMS, "AT+CSMS")                           \
    _(AT_QAUDLOOP, "AT+QAUDLOOP")                   \
    _(AT_QAUDMOD, "AT+QAUDMOD")                     \
    _(AT_CNSMOD_0, "AT+CNSMOD=0")                   \
    _(AT_CNSMOD_1, "AT+CNSMOD=1")                   \
    _(AT_CPCMFRM_8K, "AT+CPCMFRM=0")                \
    _(AT_CPCMFRM_16K, "AT+CPCMFRM=1")               \
    _(AT_VTD, "AT+VTD")                             \
    _(AT_CCID, "AT+CCID")                           \
    _(AT_CICCID, "AT+CICCID")                       \
    _(AT_QCCID, "AT+QCCID")                         \
    _(ESC, "ESC")

/* AT_COMMANDS_TABLE */

typedef enum { AT_COMMANDS_TABLE(AT_CMD_AS_ENUM) } at_cmd_t;

enum msg_status_t { MSG_STAT_REC_UNREAD, MSG_STAT_REC_READ, MSG_STAT_STO_UNSENT, MSG_STAT_STO_SENT, MSG_STAT_ALL };

struct pvt;
struct cpvt;

const char* at_cmd2str(at_cmd_t cmd);
int at_enqueue_at(struct cpvt* cpvt);
int at_enqueue_initialization(struct cpvt* cpvt);
int at_enqueue_initialization_quectel(struct cpvt*, unsigned int);
int at_enqueue_initialization_simcom(struct cpvt*);
int at_enqueue_initialization_other(struct cpvt*);
int at_enqueue_ping(struct cpvt* cpvt);
int at_enqueue_ping_taskproc(void*);
int at_enqueue_cspn_cops(struct cpvt* cpvt);
int at_enqueue_qspn_qnwinfo(struct cpvt* cpvt);
int at_enqueue_sms(struct cpvt* cpvt, const char* sca, const char* destination, const char* msg, unsigned validity_min, int report_req);
int at_enqueue_ussd(struct cpvt* cpvt, const char* code, int gsm7);
int at_enqueue_dtmf(struct cpvt* cpvt, char digit);
int at_enqueue_set_ccwa(struct cpvt* cpvt, unsigned call_waiting);
int at_enqueue_reset(struct cpvt* cpvt);
int at_enqueue_dial(struct cpvt* cpvt, const char* number, int clir);
int at_enqueue_answer(struct cpvt* cpvt);
int at_enqueue_user_cmd(struct cpvt* cpvt, const char* input);
int at_enqueue_list_messages(struct cpvt* cpvt, enum msg_status_t stat);
int at_enqueue_retrieve_sms(struct cpvt* cpvt, int idx, int sms_type);
void at_sms_retrieved(struct cpvt* cpvt, int confirm);
int at_enqueue_cmgd(struct cpvt* cpvt, unsigned int index, int delflag);
int at_enqueue_delete_sms(struct cpvt* cpvt, int idx, tristate_bool_t ack);
int at_enqueue_delete_sms_n(struct cpvt* cpvt, int idx, tristate_bool_t ack);
int at_enqueue_hangup(struct cpvt* cpvt, int call_idx, int release_cause);
int at_enqueue_volsync(struct cpvt* cpvt);
int at_enqueue_clcc(struct cpvt* cpvt);
int at_enqueue_activate(struct cpvt* cpvt);
int at_enqueue_flip_hold(struct cpvt* cpvt);
int at_enqueue_conference(struct cpvt* cpvt);
int at_hangup_immediately(struct cpvt* cpvt, int release_cause);
int at_disable_uac_immediately(struct pvt* pvt);
int at_enqueue_enable_tty(struct cpvt* cpvt);
int at_enqueue_enable_uac(struct cpvt* cpvt);
int at_enqueue_uac_apply(struct cpvt* cpvt);
int at_enqueue_mute(struct cpvt* cpvt, int mute);
int at_enqueue_qlts(struct cpvt* cpvt, int mode);
int at_enqueue_qlts_1(struct cpvt* cpvt);
int at_enqueue_cclk_query(struct cpvt* cpvt);
int at_enqueue_query_qgains(struct cpvt* cpvt);
int at_enqueue_qgains(struct cpvt* cpvt, int txgain, int rxdgain);
int at_enqueue_query_cgains(struct cpvt* cpvt);
int at_enqueue_cgains(struct cpvt* cpvt, int txgain, int rxgain);
int at_enqueue_msg_direct(struct cpvt* cpvt, int directflag);
int at_enqueue_msg_ack(struct cpvt* cpvt);
int at_enqueue_msg_ack_n(struct cpvt* cpvt, int n, int uid);
int at_enqueue_query_qaudloop(struct cpvt* cpvt);
int at_enqueue_qaudloop(struct cpvt* cpvt, int aloop);
int at_enqueue_query_qaudmod(struct cpvt* cpvt);
int at_enqueue_qaudmod(struct cpvt* cpvt, int amode);
int at_enqueue_query_qmic(struct cpvt* cpvt);
int at_enqueue_qmic(struct cpvt* cpvt, int gain);
int at_enqueue_query_qrxgain(struct cpvt* cpvt);
int at_enqueue_qrxgain(struct cpvt* cpvt, int gain);
int at_enqueue_query_cmicgain(struct cpvt* cpvt);
int at_enqueue_cmicgain(struct cpvt* cpvt, int gain);
int at_enqueue_query_coutgain(struct cpvt* cpvt);
int at_enqueue_coutgain(struct cpvt* cpvt, int gain);
int at_enqueue_cpcmreg(struct cpvt*, int);
int at_cpcmreg_immediately(struct pvt*, int);
int at_enqueue_cpcmfrm(struct cpvt*, int);
int at_enqueue_csq(struct cpvt*);
int at_enqueue_escape(struct cpvt*, int);

#endif /* CHAN_QUECTEL_AT_SEND_H_INCLUDED */
