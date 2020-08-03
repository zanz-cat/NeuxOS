#include "tty.h"
#include "keyboard.h"
#include "stdio.h"

struct console consoles[NR_CONSOLES];
struct console *cconsole = &consoles[0];

void init_console() 
{
    for (int i = 0; i < NR_CONSOLES; i++) {
        consoles[i].start = i * SCREEN_BUFFER_SIZE;
        consoles[i].limit = SCREEN_BUFFER_SIZE;
        consoles[i].screen = 0;
        consoles[i].cursor = 0;
    }
    cconsole->cursor = get_cursor();
}

void task_tty() 
{
    while (1) {
        keyboard_read();
    }
}

void in_process(u32 key) 
{
    if (key & FLAG_EXT) {
        switch (key) {
        case ENTER:
            printf("\n");
            break;
        case ESC:
            clear_screen();
            break;
        case BACKSPACE:
            printf("\b");
            break;
        case PAGEUP:
            scroll_up(cconsole);
            break;
        case PAGEDOWN:
            scroll_down(cconsole);
            break;
        default:
            break;
        }

        return;
    }
    printf("%c", (u8)key);
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
    if (c->start == c->screen) {
        return;
    }

    c->screen -= NR_SCR_COLUMNS;
    out_byte(CRT_ADDR_REG, CRT_START_H);
    out_byte(CRT_DATA_REG, c->screen >> 8);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    out_byte(CRT_DATA_REG, c->screen & 0xff);    
}

void scroll_down(struct console *c)
{
    if (c->cursor < c->screen + NR_SCR_COLUMNS * NR_SCR_ROWS) {
        return;
    }

    c->screen += NR_SCR_COLUMNS;
    out_byte(CRT_ADDR_REG, CRT_START_H);
    out_byte(CRT_DATA_REG, c->screen >> 8);
    out_byte(CRT_ADDR_REG, CRT_START_L);
    out_byte(CRT_DATA_REG, c->screen & 0xff);
}