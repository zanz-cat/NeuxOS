#include <lib/string.h>
#include <lib/log.h>
#include <lib/stdio.h>

#include <kernel/keyboard.h>
#include <kernel/io.h>
#include <kernel/printk.h>
#include <kernel/tty.h>

#define NR_TTYS         3
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
#define TTY_IN_BYTES    1024

#define out_mem_addr(out, offset) \
    ((void*)(VIDEO_MEM_BASE + 2 * ((out)->start + (offset))))

#define screen_detached(out) \
    ((out)->cursor >= (out)->screen + CRT_SIZE)

struct output {
    u32 start;
    u32 limit;
    u32 screen; // relative to start
    u32 cursor; // relative to start
    u8 color;
};

struct tty {
    u32 in_buf[TTY_IN_BYTES];
    u32 *p_inbuf_head;
    u32 *p_inbuf_tail;
    int inbuf_count;
    struct output out;
};

struct tty ttys[NR_TTYS];
int tty_current;

static void tty_do_read(int fd)
{
    if (fd == tty_current) {
        keyboard_read(fd);
    }
}

static void tty_do_write(int fd)
{
    struct tty *tty;
    char ch;

    if (fd < 0 || fd >= NR_TTYS) {
        log_error("invalid tty(%d)\n", fd);
        return;
    }

    tty = &ttys[fd];
    if (tty->inbuf_count) {
        ch = *(tty->p_inbuf_tail++);
        if (tty->p_inbuf_tail == tty->in_buf + TTY_IN_BYTES) {
            tty->p_inbuf_tail = tty->in_buf;
        }
        tty->inbuf_count--;
        tty_putchar(fd, ch);
    }
}

static u16 get_cursor() 
{
    u8 tmp;

    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_H);
    tmp = in_byte(CRT_DATA_REG);
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_L);
    return (tmp << 8) | in_byte(CRT_DATA_REG);
}

static void set_cursor(u16 pos) 
{
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_H);
    out_byte(CRT_DATA_REG, pos >> 8);
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_L);
    out_byte(CRT_DATA_REG, pos & 0xff);
}

static void scroll_up(int fd)
{
    struct output *out = &ttys[fd].out;

    if (out->screen == 0) {
        return;
    }
    out->screen -= NR_CRT_COLUMNS;
    out_byte(CRT_ADDR_REG, CRT_START_H);
    out_byte(CRT_DATA_REG, (out->start + out->screen) >> 8);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    out_byte(CRT_DATA_REG, (out->start + out->screen) & 0xff);       
}

static void scroll_down(int fd)
{
    struct output *out = &ttys[fd].out;

    if (out->cursor < out->screen + CRT_SIZE) {
        return;
    }
    out->screen += NR_CRT_COLUMNS;
    out_byte(CRT_ADDR_REG, CRT_START_H);
    out_byte(CRT_DATA_REG, (out->start + out->screen) >> 8);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    out_byte(CRT_DATA_REG, (out->start + out->screen) & 0xff);    
}

static void switch_tty(int fd)
{
    struct output *out = &ttys[fd].out;
    out_byte(CRT_ADDR_REG, CRT_START_H);
    out_byte(CRT_DATA_REG, (out->start + out->screen) >> 8);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    out_byte(CRT_DATA_REG, (out->start + out->screen) & 0xff);

    set_cursor(out->start + out->cursor);
    tty_current = fd;
}

void tty_in_process(int fd, u32 key) 
{
    if (fd < 0 || fd >= NR_TTYS) {
        log_error("invalid tty(%d)\n", fd);
        return;
    }

    struct tty *tty = &ttys[fd];
    if (key & FLAG_EXT) {
        switch (key) {
        case ENTER:
            tty_putchar(fd, '\n');
            break;
        case BACKSPACE:
            tty_putchar(fd, '\b');
            break;
        case FLAG_SHIFT_L | PAGEUP:
            for (int i = 0; i < SCROLL_ROWS; i++)
                scroll_up(fd);
            break;
        case FLAG_SHIFT_L | PAGEDOWN:
            for (int i = 0; i < SCROLL_ROWS; i++)
                scroll_down(fd);
            break;
        case F1:
            switch_tty(TTY0);
            break;
        case F2:
            switch_tty(TTY1);
            break;
        case F3:
            switch_tty(TTY2);
            break;
        default:
            break;
        }
        return;
    } else {
        if (tty->inbuf_count < TTY_IN_BYTES) {
            *(tty->p_inbuf_head++) = key;
            if (tty->p_inbuf_head == tty->in_buf + TTY_IN_BYTES) {
                tty->p_inbuf_head = tty->in_buf;
            }
            tty->inbuf_count++;
        }
    }
}

static void recycle_screen_buf(struct output *out)
{
    void *src = out_mem_addr(out, NR_CRT_COLUMNS);
    void *dst = out_mem_addr(out, 0);
    memcpy(dst, src, 2*(out->limit - NR_CRT_COLUMNS));
    for (int i = 0; i < NR_CRT_COLUMNS; i++) {
        u16 *a = (u16*)out_mem_addr(out, out->limit - NR_CRT_COLUMNS + i);
        *a = (DEFAULT_TEXT_COLOR << 8);
    }
}

int tty_putchar(int fd, char c)
{
    if (fd < 0 || fd >= NR_TTYS) {
        log_error("invalid tty(%d)\n", fd);
        return -1;
    }

    u16 *ptr;
    struct output *out = &ttys[fd].out;
    u32 cursor = out->cursor;
    switch (c) {
        case '\n':
            cursor += NR_CRT_COLUMNS - cursor % NR_CRT_COLUMNS;
            break;
        case '\b':
            if (cursor % NR_CRT_COLUMNS) {
                cursor--;
                ptr = (u16*)out_mem_addr(out, cursor);
                *ptr = (DEFAULT_TEXT_COLOR << 8);
            }
            break;
        default:
            ptr = (u16*)out_mem_addr(out, cursor);
            *ptr = (out->color << 8) | (c & 0xff);
            cursor++;
            break;
    }
    
    if (cursor == out->limit) {
        recycle_screen_buf(out);
        cursor -= NR_CRT_COLUMNS;
    }
    out->cursor = cursor;

    while (fd == tty_current && screen_detached(out)) {
        scroll_down(fd);
    }

    if (fd == tty_current) {
        set_cursor(out->start + out->cursor);
    }

    return c;
}

int tty_color(int fd, enum tty_op op, u8 *color)
{
    if (fd < 0 || fd >= NR_TTYS) {
        log_error("invalid tty(%d)\n", fd);
        return -1;
    }

    struct output *out = &ttys[fd].out;

    if (op == TTY_OP_GET) {
        *color = out->color;
    } else {
        out->color = *color;
    }
    return 0;
}

void tty_task() 
{
    int i;

    while (1) {
        for (i = 0; i < NR_TTYS; i++) {
            tty_do_read(i);
            tty_do_write(i);
        }
    }
}

void tty_init()
{
    int i;
    for (i = 0; i < NR_TTYS; i++) {
        ttys[i].inbuf_count = 0;
        ttys[i].p_inbuf_head = ttys[i].in_buf;
        ttys[i].p_inbuf_tail = ttys[i].in_buf;
        ttys[i].out.start = i * CRT_BUF_SIZE;
        ttys[i].out.limit = CRT_BUF_SIZE;
        ttys[i].out.screen = 0;
        ttys[i].out.color = DEFAULT_TEXT_COLOR;
        if (i == 0) {
            ttys[i].out.cursor = get_cursor();
        } else {
            ttys[i].out.cursor = 0;
            fprintk(i, "NeuxOS liwei-PC tty%d\n", i+1);
        }
    }
}