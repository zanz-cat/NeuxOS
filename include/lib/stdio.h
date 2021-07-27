#ifndef __STDIO_H__
#define __STDIO_H__

#define PRINTF_BUF_SIZE 1024

int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);

int vsprintf(char *buf, const char *fmt, __builtin_va_list va_arg);
#endif