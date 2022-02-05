#ifndef __KERNEL_PRINTK_H__
#define __KERNEL_PRINTK_H__

#include <stdarg.h>

void printk_set_write(int (*writer)(int, char *, size_t));

int printk(const char *fmt, ...);
int fprintk(int fd, const char *fmt, ...);

int vprintk(const char *fmt, va_list ap);
int vfprintk(int fd, const char *fmt, va_list ap);

#endif