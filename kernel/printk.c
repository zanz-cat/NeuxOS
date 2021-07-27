#include <lib/stdio.h>

#include <kernel/tty.h>
#include <kernel/printk.h>

int printk(const char *fmt, ...)
{
    int n;
    __builtin_va_list va_arg;

    __builtin_va_start(va_arg, fmt);
    n = vprintk(fmt, va_arg);
    __builtin_va_end(va_arg);
    return n;
}

int fprintk(int fd, const char *fmt, ...)
{
    int n;
    __builtin_va_list va_arg;

    __builtin_va_start(va_arg, fmt);
    n = vfprintk(fd, fmt, va_arg);
    __builtin_va_end(va_arg);
    return n;
}

int vfprintk(int fd, const char *fmt, __builtin_va_list va_arg)
{
    int i, n;
    char buf[PRINTF_BUF_SIZE];

    n = vsprintf(buf, fmt, va_arg);
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

int vprintk(const char *fmt, __builtin_va_list va_arg)
{
    int i, n;
    char buf[PRINTF_BUF_SIZE];

    n = vsprintf(buf, fmt, va_arg);
    if (n < 0) {
        return n;
    }

    for (i = 0; i < n; i++) {
        if (tty_putchar(tty_current, buf[i]) < 0) {
            break;
        }
    }
    return i;
}