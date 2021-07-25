#ifndef __PRINT_H__
#define __PRINT_H__

#include <type.h>
#include <kernel/tty.h>

void out_byte(u16 port, u8 value);
u8 in_byte(u16 port);

int fputchark(struct console *console, u32 ch);
int putsk(const char *str);
int printk(const char *fmt, ...);
int fprintk(struct console *console, const char *fmt, ...);
int vprintk(struct console *console, const char *fmt, __builtin_va_list args);

#endif