#include <stdio.h>

#include <drivers/monitor.h>

#include "tty.h"
#include "sched.h"

#include "printk.h"

#define PRINTFK_BUF_SIZE 1024

static int default_write(int fd, const char *buf, size_t n)
{
    static struct monitor _mon;
    static struct monitor *mon = NULL;
    if (mon == NULL) {
        monitor_init(&_mon, 0);
        monitor_switch(NULL, &_mon);
        mon = &_mon;
    }

    int i;
    for (i = 0; i < n; i++) {
        monitor_putchar(mon, buf[i]);
    }
    return i;
}

static int (*_write_func)(int, const char *, size_t) = default_write;

void printk_set_write(int (*writer)(int, const char *, size_t))
{
    _write_func = writer;
}

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
    int n;
    char buf[PRINTFK_BUF_SIZE];

    n = vsprintf(buf, fmt, ap);
    if (n < 0) {
        return n;
    }

    return _write_func(fd, buf, n);
}

int vprintk(const char *fmt, va_list ap)
{
    int n;
    char buf[PRINTFK_BUF_SIZE];

    n = vsprintf(buf, fmt, ap);
    if (n < 0) {
        return n;
    }

    return _write_func(TTY0, buf, n);
}