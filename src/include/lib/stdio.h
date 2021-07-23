#ifndef __STDIO_H__
#define __STDIO_H__

int printf(const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, __builtin_va_list args);

#endif