#ifndef __DRIVER_IO_H__
#define __DRIVER_IO_H__

#include <stdint.h>

void out_byte(uint16_t port, uint8_t value);
uint8_t in_byte(uint16_t port);
uint16_t in_word(uint16_t port);

#endif