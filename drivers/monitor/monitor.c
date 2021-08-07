#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <drivers/io.h>

#include "monitor.h"

#define CRT_ADDR_REG    0x3d4
#define CRT_DATA_REG    0x3d5
#define CRT_CUR_LOC_H   0xe
#define CRT_CUR_LOC_L   0xf
#define CRT_START_H     0xc
#define CRT_START_L     0xd
#define VIDEO_MEM_BASE  0xb8000

void monitor_set_cursor(uint16_t offset)
{
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_H);
    out_byte(CRT_DATA_REG, offset >> 8);
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_L);
    out_byte(CRT_DATA_REG, offset & 0xff);
}

uint16_t monitor_get_cursor()
{
    uint8_t tmp;

    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_H);
    tmp = in_byte(CRT_DATA_REG);
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_L);
    return (tmp << 8) | in_byte(CRT_DATA_REG);
}

int monitor_putchar(uint32_t offset, uint8_t color, char c)
{
    uint16_t *ptr = (uint16_t*)(VIDEO_MEM_BASE + 2 * (offset));
    *ptr = (color << 8) | (c & 0xff);
    return 0;
}

void monitor_shift(uint32_t from, int n, uint32_t limit)
{
    int i;
    uint16_t *p;
    uint32_t to = from + n;
    void *src = (uint16_t*)(VIDEO_MEM_BASE + 2 * ((from > to ? 0 : limit) + from));
    void *dst = (uint16_t*)(VIDEO_MEM_BASE + 2 * ((from > to ? 0 : limit) + to));
    size_t mov_len = limit - (n > 0 ? n : -n);

    memcpy(dst, src, 2 * mov_len);
    for (i = 0; i < (limit - mov_len); i++) {
        p = (uint16_t*)(VIDEO_MEM_BASE + 2 * ((from > to ? to + mov_len : from) + i));
        *p = (DEFAULT_TEXT_COLOR << 8);
    }
}

void monitor_set_start(uint32_t offset)
{
    out_byte(CRT_ADDR_REG, CRT_START_H);
    out_byte(CRT_DATA_REG, offset >> 8);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    out_byte(CRT_DATA_REG, offset & 0xff);
}