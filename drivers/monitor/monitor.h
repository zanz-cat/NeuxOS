#ifndef __DRIVER_MONITOR_H__
#define __DRIVER_MONITOR_H__

#define CRT_BUF_SIZE    5*1024
#define CRT_NR_COLUMNS  80
#define CRT_NR_ROWS     25
#define CRT_SIZE        (CRT_NR_COLUMNS * CRT_NR_ROWS)

#define DEFAULT_TEXT_COLOR 0x7

void monitor_set_cursor(uint16_t offset);
uint16_t monitor_get_cursor();
int monitor_putchar(uint32_t offset, uint8_t color, char c);
void monitor_shift(uint32_t from, int n, uint32_t limit);
void monitor_set_start(uint32_t offset);

#endif