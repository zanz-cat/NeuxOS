#ifndef __STDIO_H__
#define __STDIO_H__

#include "type.h"

#define DEFAULT_TEXT_COLOR 0x7
#define DEFAULT_PRINT_BUF_SIZE 1024

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