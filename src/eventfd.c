/*
    eventfd.c
*/

#include <errno.h>
#include <unistd.h>

#include "eventfd.h"

int eventfd_create() { return eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK); }

int eventfd_signal(int fd) { return eventfd_write(fd, 1); }

int eventfd_reset(int fd)
{
    eventfd_t v;
    const int res = eventfd_read(fd, &v);
    if (res && errno != EAGAIN) {
        return -1;
    }
    // EAGAIN is non-fatal here since that just means that
    // the event was already reset when we were called,
    // which is ok
    return 0;
}

void eventfd_close(int* fd)
{
    if (!fd) {
        return;
    }
    if (*fd < 0) {
        return;
    }

    close(*fd);
    *fd = -1;
}
