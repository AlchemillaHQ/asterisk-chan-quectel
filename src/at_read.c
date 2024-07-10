/*
   Copyright (C) 2009 - 2010

   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org

   Dmitry Vagin <dmitry2004@yandex.ru>

   Copyright (C) 2010 - 2011
   bg <bg_one@mail.ru>
*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* vasprintf() in asterisk/utils.h */
#endif              /* #ifndef _GNU_SOURCE */

#include <errno.h>
#include <sys/types.h>

#include "ast_config.h"

#include <asterisk/channel.h> /* ast_waitfor_n_fd() */
#include <asterisk/logger.h>  /* ast_debug() */

#include "at_read.h"

#include "chan_quectel.h"
#include "helpers.h"
#include "ringbuffer.h"

/*!
 * \brief Wait for activity on an socket
 * \param fd -- file descriptor
 * \param ms  -- pointer to an int containing a timeout in ms
 * \return 0 on timeout and the socket fd (non-zero) otherwise
 * \retval 0 timeout
 */

int at_wait(int fd, int* ms)
{
    int exception;

    const int outfd = ast_waitfor_n_fd(&fd, 1, ms, &exception);

    if (outfd < 0) {
        return 0;
    }

    return outfd;
}

#/* return number of bytes read */

ssize_t at_read(const char* dev, int fd, struct ringbuffer* rb)
{
    struct iovec iov[2];
    ssize_t n = -1;

    /* TODO: read until major error */
    int iovcnt = rb_write_iov(rb, iov);

    if (iovcnt > 0) {
        n = readv(fd, iov, iovcnt);

        if (n < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                ast_debug(1, "[%s] readv() error: %d\n", dev, errno);
                return n;
            }

            return 0;
        } else if (n > 0) {
            rb_write_upd(rb, n);

            ast_debug(6, "[%s] receive %zu byte, used %zu, free %zu, read %zu, write %zu\n", dev, n, rb_used(rb), rb_free(rb), rb->read, rb->write);

            iovcnt = rb_read_all_iov(rb, iov);

            if (iovcnt > 0) {
                if (iovcnt == 2) {
                    ast_debug(5, "[%s] [%u+%u][%s%s]\n", dev, (unsigned)iov[0].iov_len, (unsigned)iov[1].iov_len, tmp_esc_nstr(iov[0].iov_base, iov[0].iov_len),
                              tmp_esc_nstr(iov[1].iov_base, iov[1].iov_len));
                } else {
                    ast_debug(5, "[%s] [%u][%s]\n", dev, (unsigned)iov[0].iov_len, tmp_esc_nstr(iov[0].iov_base, iov[0].iov_len));
                }
            }
        }
    } else {
        ast_log(LOG_ERROR, "[%s] at cmd receive buffer overflow\n", dev);
    }
    return n;
}

/* anybody wrote some to device before me, and not read results, clean pending results here */
void at_clean_data(const char* dev, int fd, struct ringbuffer* const rb)
{
    rb_reset(rb);
    for (int t = 0; at_wait(fd, &t); t = 0) {
        const int iovcnt = at_read(dev, fd, rb);
        if (iovcnt) {
            ast_debug(4, "[%s] Drop %u bytes of pending data\n", dev, (unsigned)rb_used(rb));
        }
        /* drop read */
        rb_reset(rb);
        if (!iovcnt) {
            break;
        }
    }
}

size_t at_get_iov_size(const struct iovec* iov) { return iov[0].iov_len + iov[1].iov_len; }

size_t at_get_iov_size_n(const struct iovec* iov, int iovcnt)
{
    size_t res = 0u;
    for (int i = 0; i < iovcnt; ++i) {
        res += iov[i].iov_len;
    }
    return res;
}

size_t at_combine_iov(struct ast_str* const result, const struct iovec* const iov, int iovcnt)
{
    const size_t len = at_get_iov_size_n(iov, iovcnt);

    ast_str_truncate(result, len);
    if (iovcnt > 0) {
        char* const buf = ast_str_buffer(result);
        memcpy(buf, iov[0].iov_base, iov[0].iov_len);
        if (iovcnt > 1) {
            memcpy(buf + iov[0].iov_len, iov[1].iov_base, iov[1].iov_len);
        }
    }

    return len;
}

static size_t get_2ndeol_pos(const struct ringbuffer* const rb, struct iovec* iov, struct ast_str* buf)
{
    static const char EOL[2] = {'\r', '\n'};

    const int iovcnt = rb_read_all_iov(rb, iov);
    if (!iovcnt) {
        return 0u;
    }

    const size_t len    = at_combine_iov(buf, iov, iovcnt);
    const char* const b = ast_str_buffer(buf);

    const char* pos = (const char*)memmem(b, len, EOL, ARRAY_LEN(EOL));
    if (!pos) {
        return 0u;
    }

    const size_t shift = (pos - b) + ARRAY_LEN(EOL);
    pos                = (const char*)memmem(b + shift, len - shift, EOL, ARRAY_LEN(EOL));
    if (!pos) {
        return 0u;
    }

    return pos - b;
}

int at_read_result_iov(const char* dev, int* read_result, size_t* skip, struct ringbuffer* rb, struct iovec* iov, struct ast_str* buf)
{
    static const char M_CMGR[]       = "+CMGR:";
    static const char M_CNUM[]       = "+CNUM:";
    static const char M_ERROR_CNUM[] = "ERROR+CNUM:";
    static const char M_CMGL[]       = "+CMGL:";
    static const char M_EOL[]        = "\r\n";
    static const char M_SMS_PROMPT[] = "> ";
    static const char M_CMT[]        = "+CMT:";
    static const char M_CBM[]        = "+CBM:";
    static const char M_CDS[]        = "+CDS:";
    static const char M_CLASS0[]     = "+CLASS0:";

    static const char T_OK[]   = "\r\n\r\nOK\r\n";
    static const char T_CMGL[] = "\r\n+CMGL:";

    size_t s = rb_used(rb);

    if (s > 0) {
        /*		ast_debug (5, "[%s] d_read_result %d len %d input [%.*s]\n", dev, *read_result, s, MIN(s, rb->size -
         * rb->read), (char*)rb->buffer + rb->read); */

        if (*read_result == 0) {
            const int res = rb_memcmp(rb, M_EOL, STRLEN(M_EOL));
            if (!res) {
                rb_read_upd(rb, STRLEN(M_EOL));
                *read_result = 1;

                return at_read_result_iov(dev, read_result, skip, rb, iov, buf);
            } else if (res > 0) {
                if (rb_read_is_printable(rb)) {
                    *read_result = 1;
                    return at_read_result_iov(dev, read_result, skip, rb, iov, buf);
                }

                if (!rb_memcmp(rb, "\n", 1)) {
                    rb_read_upd(rb, 1);
                    return at_read_result_iov(dev, read_result, skip, rb, iov, buf);
                }

                if (rb_read_until_char_iov(rb, iov, '\r')) {
                    s = at_get_iov_size(iov);
                    rb_read_upd(rb, s + 1u);
                    return at_read_result_iov(dev, read_result, skip, rb, iov, buf);
                }

                rb_read_upd(rb, s);
                return at_read_result_iov(dev, read_result, skip, rb, iov, buf);
            }

            return 0;
        } else {
            if (!rb_memcmp(rb, M_SMS_PROMPT, STRLEN(M_SMS_PROMPT))) {
                *read_result = 0;
                return rb_read_n_iov(rb, iov, STRLEN(M_SMS_PROMPT));
            } else if (!(rb_memcmp(rb, M_CMGR, STRLEN(M_CMGR)) && rb_memcmp(rb, M_CNUM, STRLEN(M_CNUM)) && rb_memcmp(rb, M_ERROR_CNUM, STRLEN(M_ERROR_CNUM)))) {
                const int iovcnt = rb_read_until_mem_iov(rb, iov, T_OK, STRLEN(T_OK));
                if (iovcnt) {
                    *skip += 4;
                }

                return iovcnt;
            } else if (!rb_memcmp(rb, M_CMGL, STRLEN(M_CMGL))) {
                int iovcnt = rb_read_until_mem_iov(rb, iov, T_CMGL, STRLEN(T_CMGL));
                if (iovcnt) {
                    *skip += 2;
                } else {
                    iovcnt = rb_read_until_mem_iov(rb, iov, T_OK, STRLEN(T_OK));
                    if (iovcnt) {
                        *skip += 4;
                    }
                }
                return iovcnt;
            } else if (!(rb_memcmp(rb, M_CMT, STRLEN(M_CMT)) && rb_memcmp(rb, M_CBM, STRLEN(M_CBM)) && rb_memcmp(rb, M_CDS, STRLEN(M_CDS)) &&
                         rb_memcmp(rb, M_CLASS0, STRLEN(M_CLASS0)))) {
                s = get_2ndeol_pos(rb, iov, buf);
                if (s) {
                    *read_result  = 0;
                    *skip        += 1;
                    return rb_read_n_iov(rb, iov, s);
                }
            } else {
                const int iovcnt = rb_read_until_mem_iov(rb, iov, M_EOL, STRLEN(M_EOL));
                if (iovcnt) {
                    *read_result  = 0;
                    *skip        += 1;
                    return iovcnt;
                }
            }
        }
    }

    return 0;
}
