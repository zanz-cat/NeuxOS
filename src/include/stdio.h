#ifndef __STDIO_H__
#define __STDIO_H__

#include "type.h"

void out_byte(u16 port, u8 value);
u8 in_byte(u16 port);

int set_text_color(u8 color);
int get_text_color();
int reset_text_color();

int backspace();
int putchar(int ch);
int puts(const char *str);
int printf(const char *fmt, ...);

#endif