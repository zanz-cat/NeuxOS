#include "tty.h"
#include "keyboard.h"
#include "print.h"
#include "string.h"

struct console console_table[NR_CONSOLES];
struct tty  tty_table[NR_CONSOLES];
struct console *current_console = &console_table[0];

static void tty_do_read(struct tty *tty)
{
    if (is_current_console(tty->console))
        keyboard_read(tty);
}

static void tty_do_write(struct tty *tty)
{
    if (tty->inbuf_count) {
        char ch = *(tty->p_inbuf_tail++);
        if (tty->p_inbuf_tail == tty->in_buf + TTY_IN_BYTES) {
            tty->p_inbuf_tail = tty->in_buf;
        }
        tty->inbuf_count--;
        fputchark(tty->console, ch);
    }
}

int is_current_console(struct console *p_con)
{
    return (p_con == current_console);
}

void init_console() 
{
    for (int i = 0; i < NR_CONSOLES; i++) {
        console_table[i].start = i * CRT_BUF_SIZE;
        console_table[i].limit = CRT_BUF_SIZE;
        console_table[i].screen = 0;
        console_table[i].color = DEFAULT_TEXT_COLOR;
        memset(console_table[i].print_buf, 0, sizeof(console_table[i].print_buf));
        if (i == 0) {
            console_table[i].cursor = get_cursor();
        } else {
            console_table[i].cursor = 0;
            fprintk(&console_table[i], "SeeOS liwei-PC tty%d\n", i+1);
        }
    }
}

void init_tty()
{
    for (int i = 0; i < NR_CONSOLES; i++) {
        tty_table[i].inbuf_count = 0;
        tty_table[i].p_inbuf_head = tty_table[i].in_buf;
        tty_table[i].p_inbuf_tail = tty_table[i].in_buf;
        tty_table[i].console = &console_table[i];
    }
}

void task_tty() 
{
    while (1) {
        for (int i = 0; i < NR_CONSOLES; i++) {
            struct tty *tty = &tty_table[i];
            tty_do_read(tty);
            tty_do_write(tty);
        }
    }
}

u16 get_cursor() 
{    
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_H);
    u8 tmp = in_byte(CRT_DATA_REG);
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_L);
    return (tmp << 8) | in_byte(CRT_DATA_REG);
}

void set_cursor(u16 pos) 
{
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_H);
    out_byte(CRT_DATA_REG, pos >> 8);
    out_byte(CRT_ADDR_REG, CRT_CUR_LOC_L);
    out_byte(CRT_DATA_REG, pos & 0xff);
}

void scroll_up(struct console *c)
{
    if (0 == c->screen)
        return;

    c->screen -= NR_CRT_COLUMNS;
    out_byte(CRT_ADDR_REG, CRT_START_H);
    out_byte(CRT_DATA_REG, (c->start + c->screen) >> 8);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    out_byte(CRT_DATA_REG, (c->start + c->screen) & 0xff);       
}

void scroll_down(struct console *c)
{
    if (c->cursor < c->screen + CRT_SIZE)
        return;

    c->screen += NR_CRT_COLUMNS;
    out_byte(CRT_ADDR_REG, CRT_START_H);
    out_byte(CRT_DATA_REG, (c->start + c->screen) >> 8);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    out_byte(CRT_DATA_REG, (c->start + c->screen) & 0xff);    
}

void switch_tty(struct console *c)
{
    out_byte(CRT_ADDR_REG, CRT_START_H);
    out_byte(CRT_DATA_REG, (c->start + c->screen) >> 8);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    out_byte(CRT_DATA_REG, (c->start + c->screen) & 0xff);

    set_cursor(c->start + c->cursor);
    current_console = c;
}

void in_process(struct tty *tty, u32 key) 
{
    if (key & FLAG_EXT) {
        switch (key) {
        case ENTER:
            fprintk(tty->console, "\n");
            break;
        case BACKSPACE:
            fprintk(tty->console, "\b");
            break;
        case FLAG_SHIFT_L | PAGEUP:
            for (int i = 0; i < SCROLL_ROWS; i++)
                scroll_up(tty->console);
            break;
        case FLAG_SHIFT_L | PAGEDOWN:
            for (int i = 0; i < SCROLL_ROWS; i++)
                scroll_down(tty->console);
            break;
        case F1:
            switch_tty(&console_table[TTY1_INDEX]);
            break;
        case F2:
            switch_tty(&console_table[TTY2_INDEX]);
            break;
        case F3:
            switch_tty(&console_table[TTY3_INDEX]);
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

struct tty *get_tty(int index) 
{
    return &tty_table[index];
}