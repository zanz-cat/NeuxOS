#include "tty.h"
#include "keyboard.h"
#include "stdio.h"
#include "string.h"

struct console console_table[NR_CONSOLES];
struct console *current_console = &console_table[0];

void init_console() 
{
    for (int i = 0; i < NR_CONSOLES; i++) {
        console_table[i].start = i * CRT_BUF_SIZE;
        console_table[i].limit = CRT_BUF_SIZE;
        console_table[i].screen = 0;
        if (i == 0) {
            console_table[i].cursor = get_cursor();
        } else {
            console_table[i].cursor = 0;
            fprintf(&console_table[i], "LeeOS liwei-PC tty%d\n", i+1);
        }
        console_table[i].color = DEFAULT_TEXT_COLOR;
        memset(console_table[i].print_buf, 0, sizeof(console_table[i].print_buf));
    }
}

void task_tty() 
{
    while (1) {
        keyboard_read();
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
    if (0 == c->screen) {
        return;
    }

    c->screen -= NR_CRT_COLUMNS;
    out_byte(CRT_ADDR_REG, CRT_START_H);
    out_byte(CRT_DATA_REG, (c->start + c->screen) >> 8);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    out_byte(CRT_DATA_REG, (c->start + c->screen) & 0xff);    
}

void scroll_down(struct console *c)
{
    if (c->cursor < c->screen + CRT_SIZE) {
        return;
    }

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

void in_process(u32 key) 
{
    if (key & FLAG_EXT) {
        switch (key) {
        case ENTER:
            printf("\n");
            break;
        case BACKSPACE:
            backspace();
            break;
        case FLAG_SHIFT_L | PAGEUP:
            for (int i = 0; i < SCROLL_ROWS; i++)
                scroll_up(current_console);
            break;
        case FLAG_SHIFT_L | PAGEDOWN:
            for (int i = 0; i < SCROLL_ROWS; i++)
                scroll_down(current_console);
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
    }
    printf("%c", (u8)key);
}

struct console *get_console(int index) 
{
    return &console_table[index];
}