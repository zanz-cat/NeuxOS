#ifndef __KERNEL_PRINTK_H__
#define __KERNEL_PRINTK_H__

int printk(const char *fmt, ...);
int fprintk(int fd, const char *fmt, ...);

int vprintk(const char *fmt, __builtin_va_list va_arg);
int vfprintk(int fd, const char *fmt, __builtin_va_list va_arg);

#endif