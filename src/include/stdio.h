#ifndef __STDIO_H__
#define __STDIO_H__

#include "tty.h"

void out_byte(u16 port, u8 value);
u8 in_byte(u16 port);

int clear_screen();
int backspace();
int putchar(int ch);
int fputchar(struct console *console, u32 ch);
int puts(const char *str);
int printf(const char *fmt, ...);
int vprintf(struct console *console, const char *fmt, __builtin_va_list args);
int fprintf(struct console *console, const char *fmt, ...);

#endif