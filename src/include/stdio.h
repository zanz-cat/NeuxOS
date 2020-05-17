#ifndef __STDIO_H__
#define __STDIO_H__

#include "type.h"

void out_byte(u16 port, u8 value);
u8 in_byte(u16 port);

int set_text_color(u8 color);
int get_text_color();
int reset_text_color();

int clear_screen();
int backspace();
int putchar(int ch);
int puts(const char *str);
int printf(const char *fmt, ...);

int putchar_pos(int ch, u16 pos);
int puts_pos(const char *str, u16 pos);
int printf_pos(u16 pos, const char *fmt, ...);

#endif