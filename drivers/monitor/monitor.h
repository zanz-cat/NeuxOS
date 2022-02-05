#ifndef __DRIVER_MONITOR_H__
#define __DRIVER_MONITOR_H__

#include <stdint.h>
#include <stdbool.h>

#define CRT_BUF_SIZE    5*1024
#define CRT_NR_COLUMNS  80
#define CRT_NR_ROWS     25
#define CRT_SIZE        (CRT_NR_COLUMNS * CRT_NR_ROWS)

#define DEFAULT_TEXT_COLOR 0x7
#define MONITOR_PHY_MEM 0xb8000

struct monitor {
    uint16_t start;
    uint16_t limit;
    uint16_t screen; // display area, relative to start
    uint16_t cursor; // relative to start
    uint8_t color;
    bool foreground;
};

void monitor_init(struct monitor *mon, int index);
void monitor_scroll_up(struct monitor *mon);
void monitor_scroll_down(struct monitor *mon);
void monitor_switch(struct monitor *old, struct monitor *mon);
int monitor_putchar(struct monitor *mon, char c);

#endif