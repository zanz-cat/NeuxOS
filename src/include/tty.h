#ifndef __TTY_H__
#define __TTY_H__
#include "type.h"

#define NR_CONSOLES     3
#define CRT_BUF_SIZE    5*1024

#define CRT_ADDR_REG    0x3d4
#define CRT_DATA_REG    0x3d5
#define CRT_CUR_LOC_H   0xe
#define CRT_CUR_LOC_L   0xf
#define CRT_START_H     0xc
#define CRT_START_L     0xd

#define NR_CRT_COLUMNS  80
#define NR_CRT_ROWS     25
#define CRT_SIZE        (NR_CRT_COLUMNS * NR_CRT_ROWS)

#define SCROLL_ROWS     15

#define VIDEO_MEM_BASE 0xb8000

#define DEFAULT_PRINT_BUF_SIZE 1024

#define DEFAULT_TEXT_COLOR 0x7

#define TTY_IN_BYTES    1024
#define TTY1_INDEX      0
#define TTY2_INDEX      1
#define TTY3_INDEX      2

struct console {
    u32 start;
    u32 limit;
    u32 screen; // relative to start
    u32 cursor; // relative to start
    u8 color;
    char print_buf[DEFAULT_PRINT_BUF_SIZE];
};

struct tty {
    u32 in_buf[TTY_IN_BYTES];
    u32 *p_inbuf_head;
    u32 *p_inbuf_tail;
    int inbuf_count;

    struct console *console;
};

void init_console();
void init_tty();
int is_current_console(struct console *p_con);
void task_tty();
void in_process(struct tty *tty, u32 key);
void scroll_up(struct console *c);
void scroll_down(struct console *c);
void set_cursor(u16 pos);
u16 get_cursor();

struct tty *get_tty(int index);

extern struct console *current_console;

#define console_mem_addr(console, offset) \
    ((void*)(VIDEO_MEM_BASE + 2 * ((console)->start + (offset))))

#define screen_detached(console) \
    ((console)->cursor >= (console)->screen + CRT_SIZE)

#endif
