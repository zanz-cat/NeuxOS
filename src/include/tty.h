#ifndef __TTY_H__
#define __TTY_H__
#include "type.h"

#define NR_CONSOLES 3
#define SCREEN_BUFFER_SIZE 10*1024

#define CRT_ADDR_REG    0x3d4
#define CRT_DATA_REG    0x3d5
#define CRT_CUR_LOC_H   0xe
#define CRT_CUR_LOC_L   0xf
#define CRT_START_H     0xc
#define CRT_START_L     0xd

#define NR_SCR_COLUMNS  80
#define NR_SCR_ROWS     25

#define VIDEO_MEM_START 0xb8000

struct console {
    u32 start;
    u32 limit;
    u32 screen; // relative to start
    u32 cursor; // relative to start
};

void init_console();
void task_tty();
void in_process(u32 key);
u16 get_cursor();
void set_cursor(u16 pos);
void scroll_up(struct console *c);
void scroll_down(struct console *c);

/* current console */
extern struct console *cconsole;

#define VIDEO_MEM_ADDR(c) (VIDEO_MEM_START + 2 * c->start + 2 * c->cursor)
#endif
