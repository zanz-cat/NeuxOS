#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <drivers/io.h>
#include <kernel/memory.h>
#include <config.h>

#include "monitor.h"

#define CRT_ADDR_REG    0x3d4
#define CRT_DATA_REG    0x3d5
#define CRT_CUR_LOC_H   0xe
#define CRT_CUR_LOC_L   0xf
#define CRT_START_H     0xc
#define CRT_START_L     0xd

#ifdef NO_PAGE
    #define VIDEO_MEM_BASE  ((uint16_t *)(MONITOR_PHY_MEM))
#else
    #define VIDEO_MEM_BASE  ((uint16_t *)virt_addr(MONITOR_PHY_MEM))
#endif

#define screen_detached(mon) \
    ((mon)->cursor >= (mon)->screen + CRT_SIZE)

static void _set_cursor(uint16_t offset)
{
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_H);
    out_byte(CRT_DATA_REG, offset >> 8);
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_L);
    out_byte(CRT_DATA_REG, offset & 0xff);
}

static uint16_t _get_cursor()
{
    uint8_t tmp;

    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_H);
    tmp = in_byte(CRT_DATA_REG);
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_L);
    return (tmp << 8) | in_byte(CRT_DATA_REG);
}

static void _set_start(uint16_t offset)
{
    out_byte(CRT_ADDR_REG, CRT_START_H);
    out_byte(CRT_DATA_REG, offset >> 8);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    out_byte(CRT_DATA_REG, offset & 0xff);
}

static uint16_t _get_start()
{
    uint16_t tmp;

    out_byte(CRT_ADDR_REG, CRT_START_H);
    tmp = in_byte(CRT_DATA_REG);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    return (tmp << 8) | in_byte(CRT_DATA_REG);
}

static int _monitor_putchar(uint16_t offset, uint8_t color, char c)
{
    uint16_t *ptr = VIDEO_MEM_BASE + offset;
    *ptr = (color << 8) | (c & 0xff);
    return 0;
}

static void shift(uint16_t from, int n, uint16_t limit)
{
    int i;
    uint16_t *p;
    uint16_t to = from + n;
    void *src = VIDEO_MEM_BASE + (from > to ? 0 : limit) + from;
    void *dest = VIDEO_MEM_BASE + (from > to ? 0 : limit) + to;
    size_t len = limit - abs(n);

    memmove(dest, src, 2 * len);
    for (i = 0; i < (limit - len); i++) {
        p = VIDEO_MEM_BASE + (from > to ? to + len : from) + i;
        *p = (DEFAULT_TEXT_COLOR << 8);
    }
}

int monitor_putchar(struct monitor *mon, char c)
{
    uint32_t cursor = mon->cursor;
    switch (c) {
        case '\n':
            cursor += CRT_NR_COLUMNS - cursor % CRT_NR_COLUMNS;
            break;
        case '\b':
            cursor--;
            _monitor_putchar(mon->start + cursor, DEFAULT_TEXT_COLOR, '\0');
            break;
        default:
            _monitor_putchar(mon->start + cursor, mon->color, c);
            cursor++;
            break;
    }

    if (cursor == mon->limit) {
        shift(mon->start + CRT_NR_COLUMNS, -CRT_NR_COLUMNS, mon->limit);
        cursor -= CRT_NR_COLUMNS;
    }
    mon->cursor = cursor;

    if (mon->foreground) {
        while (screen_detached(mon)) {
            monitor_scroll_down(mon);
        }
        _set_cursor(mon->start + mon->cursor);
    }
    return c;
}

void monitor_init(struct monitor *mon, int index)
{
    mon->start = index * CRT_BUF_SIZE;
    mon->limit = CRT_BUF_SIZE;
    mon->screen = index == 0 ? _get_start() : 0;
    mon->color = DEFAULT_TEXT_COLOR;
    mon->cursor = index == 0 ? _get_cursor() : 0;
    mon->foreground = false;
}

void monitor_scroll_up(struct monitor *mon)
{
    if (mon->screen == 0) {
        return;
    }
    mon->screen -= CRT_NR_COLUMNS;
    _set_start(mon->start + mon->screen);
}

void monitor_scroll_down(struct monitor *mon)
{
    if (mon->cursor < mon->screen + CRT_SIZE) {
        return;
    }
    mon->screen += CRT_NR_COLUMNS;
    _set_start(mon->start + mon->screen);
}

void monitor_switch(struct monitor *old, struct monitor *mon)
{
    if (old != NULL) {
        old->foreground = false;
    }

    mon->foreground = true;
    _set_start(mon->start + mon->screen);
    _set_cursor(mon->start + mon->cursor);
    while (screen_detached(mon)) {
        monitor_scroll_down(mon);
    }
}