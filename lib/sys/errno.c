#include <stdio.h>
#include <stdlib.h>

#include "lib/misc.h"

#include <errno.h>

int *__errno_location(void)
{
    static int __errno;
    return &__errno;
}

char *strerror(int errnum)
{
    static char *errmsg[] = {
        [EPERM] = "Operation not permitted",
        [ENOENT] = "No such file or directory",
        [ENOTDIR] = "Not a directory",
        [EINVAL] = "Invalid argument",
        [ENOTSUP] = "Operation not supported",
        [ENAMETOOLONG] = "File name too long",
        [EBADFD] = "File descriptor in bad state",
        [ETIMEDOUT] = "Connection timed out",
        [ENOBUFS] = "No buffer space available",
    };
    static char unknown[16];

    int i = abs(errnum);
    if (i < array_size(errmsg) && errmsg[i] != NULL) {
        return errmsg[i];
    }

    sprintf(unknown, "errno=%d", -i);
    return unknown;
}