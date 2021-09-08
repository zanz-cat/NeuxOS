#include <string.h>
#include <stdio.h>

#include <lib/log.h>

#include <drivers/io.h>
#include <drivers/monitor.h>
#include <drivers/keyboard.h>

#include "printk.h"
#include "sched.h"

#include "tty.h"

#define TTYS_COUNT (TTY_MAX+1)
#define SCROLL_ROWS 15
#define TTY_IN_BYTES 1024

struct output {
    uint32_t start;
    uint32_t limit;
    uint32_t screen; // display area, relative to start
    uint32_t cursor; // relative to start
    uint8_t color;
};

struct tty {
    uint32_t in_buf[TTY_IN_BYTES];
    uint32_t *p_inbuf_head;
    uint32_t *p_inbuf_tail;
    int inbuf_count;
    struct output out;
};

#define screen_detached(out) \
    ((out)->cursor >= (out)->screen + CRT_SIZE)

struct tty ttys[TTYS_COUNT];
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

static void scroll_up(int fd)
{
    struct output *out = &ttys[fd].out;

    if (out->screen == 0) {
        return;
    }
    out->screen -= CRT_NR_COLUMNS;
    monitor_set_start(out->start + out->screen);
}

static void scroll_down(int fd)
{
    struct output *out = &ttys[fd].out;

    if (out->cursor < out->screen + CRT_SIZE) {
        return;
    }
    out->screen += CRT_NR_COLUMNS;
    monitor_set_start(out->start + out->screen);
}

static void switch_tty(int fd)
{
    struct output *out = &ttys[fd].out;
    monitor_set_start(out->start + out->screen);
    monitor_set_cursor(out->start + out->cursor);
    tty_current = fd;
    while (screen_detached(out)) {
        scroll_down(fd);
    }
}

void tty_in_proc(int fd, uint32_t key)
{
    struct tty *tty;

    if (fd < TTY0 || fd > TTY_MAX) {
        return;
    }
    tty = &ttys[fd];
    if (key & FLAG_EXT) {
        switch (key) {
        case ENTER:
            tty_putchar(fd, '\n');
            break;
        case BACKSPACE:
            tty_putchar(fd, '\b');
            break;
        case FLAG_SHIFT_L | PAGEUP:
            for (int i = 0; i < SCROLL_ROWS; i++) {
                scroll_up(fd);
            }
            break;
        case FLAG_SHIFT_L | PAGEDOWN:
            for (int i = 0; i < SCROLL_ROWS; i++) {
                scroll_down(fd);
            }
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
        case F9:
            system_load_report();
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

int tty_putchar(int fd, char c)
{
    if (fd == TTY_NULL) {
        return c;
    }
    if (fd < TTY_NULL || fd > TTY_MAX) {
        log_error("invalid tty %d\n", fd);
        return c;
    }

    struct output *out = &ttys[fd].out;
    uint32_t cursor = out->cursor;
    switch (c) {
        case '\n':
            cursor += CRT_NR_COLUMNS - cursor % CRT_NR_COLUMNS;
            break;
        case '\b':
            if (cursor % CRT_NR_COLUMNS) {
                cursor--;
                monitor_putchar(out->start + cursor, DEFAULT_TEXT_COLOR, '\0');
            }
            break;
        default:
            monitor_putchar(out->start + cursor, out->color, c);
            cursor++;
            break;
    }

    if (cursor == out->limit) {
        monitor_shift(out->start + CRT_NR_COLUMNS, -CRT_NR_COLUMNS, out->limit);
        cursor -= CRT_NR_COLUMNS;
    }
    out->cursor = cursor;

    while (fd == tty_current && screen_detached(out)) {
        scroll_down(fd);
    }

    if (fd == tty_current) {
        monitor_set_cursor(out->start + out->cursor);
    }

    return c;
}

int tty_color(int fd, enum tty_op op, uint8_t *color)
{
    if (fd == TTY_NULL) {
        return 0;
    }
    if (fd < TTY_NULL || fd > TTY_MAX) {
        log_error("invalid tty %d\n", fd);
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
        for (i = 0; i < TTYS_COUNT; i++) {
            tty_do_read(i);
            tty_do_write(i);
        }
    }
}

void tty_setup()
{
    int i;
    for (i = 0; i < TTYS_COUNT; i++) {
        ttys[i].inbuf_count = 0;
        ttys[i].p_inbuf_head = ttys[i].in_buf;
        ttys[i].p_inbuf_tail = ttys[i].in_buf;
        ttys[i].out.start = i * CRT_BUF_SIZE;
        ttys[i].out.limit = CRT_BUF_SIZE;
        ttys[i].out.screen = 0;
        ttys[i].out.color = DEFAULT_TEXT_COLOR;
        if (i == 0) {
            ttys[i].out.cursor = monitor_get_cursor();
        } else {
            ttys[i].out.cursor = 0;
            fprintk(i, "NeuxOS liwei-PC tty%d\n", i+1);
        }
    }
}