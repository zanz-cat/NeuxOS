#ifndef __STDIO_H__
#define __STDIO_H__

#include "type.h"

u32 putchar(u32 ch);
u32 color_putchar(u32 ch, u8 color);
u32 puts(const u8* str);
u32 color_puts(const u8* str, u8 color);

#endif