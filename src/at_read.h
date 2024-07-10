/*
   Copyright (C) 2010 bg <bg_one@mail.ru>
*/
#ifndef CHAN_QUECTEL_AT_READ_H_INCLUDED
#define CHAN_QUECTEL_AT_READ_H_INCLUDED

struct pvt;
struct ringbuffer;
struct iovec;

int at_wait(int fd, int* ms);
ssize_t at_read(const char* dev, int fd, struct ringbuffer* rb);
void at_clean_data(const char* dev, int fd, struct ringbuffer* const rb);

size_t at_get_iov_size(const struct iovec* iov);
size_t at_get_iov_size_n(const struct iovec* iov, int iovcnt);

size_t at_combine_iov(struct ast_str* const, const struct iovec* const, int);

int at_read_result_iov(const char* dev, int* read_result, size_t* skip, struct ringbuffer* rb, struct iovec* iov, struct ast_str* buf);

#endif /* CHAN_QUECTEL_AT_READ_H_INCLUDED */
