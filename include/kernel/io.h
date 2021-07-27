#ifndef __KERNEL_IO_H__
#define __KERNEL_IO_H__

#include <type.h>

void out_byte(u16 port, u8 value);
u8 in_byte(u16 port);

#endif