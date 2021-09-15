#include <stdio.h>

#include "tty.h"
#include "sched.h"

#include "printk.h"

#define PRINTFK_BUF_SIZE 1024

int printk(const char *fmt, ...)
{
    int n;
    va_list ap;

    va_start(ap, fmt);
    n = vprintk(fmt, ap);
    va_end(ap);
    return n;
}

int fprintk(int fd, const char *fmt, ...)
{
    int n;
    va_list ap;

    va_start(ap, fmt);
    n = vfprintk(fd, fmt, ap);
    va_end(ap);
    return n;
}

int vfprintk(int fd, const char *fmt, va_list ap)
{
    int i, n;
    char buf[PRINTFK_BUF_SIZE];

    n = vsprintf(buf, fmt, ap);
    if (n < 0) {
        return n;
    }

    for (i = 0; i < n; i++) {
        if (tty_putchar(fd, buf[i]) < 0) {
            break;
        }
    }
    return i;
}

int vprintk(const char *fmt, va_list ap)
{
    int i, n, fd;
    char buf[PRINTFK_BUF_SIZE];

    n = vsprintf(buf, fmt, ap);
    if (n < 0) {
        return n;
    }

    if (current == NULL || current->tty == TTY_NULL) {
        fd = TTY0;
    } else {
        fd = current->tty;
    }
    for (i = 0; i < n; i++) {
        if (tty_putchar(fd, buf[i]) < 0) {
            break;
        }
    }
    return i;
}