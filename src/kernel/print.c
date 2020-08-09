#include "print.h"
#include "string.h"
#include "common.h"
#include "sched.h"

static void recycle_screen_buf(struct console *c)
{
    void *src = console_mem_addr(c, NR_CRT_COLUMNS);
    void *dst = console_mem_addr(c, 0);
    memcpy(dst, src, 2*(c->limit - NR_CRT_COLUMNS));
    for (int i = 0; i < NR_CRT_COLUMNS; i++) {
        u16 *a = (u16*)console_mem_addr(c, c->limit - NR_CRT_COLUMNS + i);
        *a = (DEFAULT_TEXT_COLOR << 8);
    }
}

int fputchark(struct console *console, u32 ch) 
{
    u16 *ptr;
    u32 tmp;
    u32 cursor = console->cursor;
    switch (ch)
    {
    case '\n':
        cursor += NR_CRT_COLUMNS - cursor % NR_CRT_COLUMNS;
        break;
    case '\b':
        if (cursor % NR_CRT_COLUMNS) {
            cursor--;
            ptr = (u16*)console_mem_addr(console, cursor);
            *ptr = (DEFAULT_TEXT_COLOR << 8);
        }
        break;
    default:
        ptr = (u16*)console_mem_addr(console, cursor);
        *ptr = (console->color << 8) | (ch & 0xff);
        cursor++;
        break;
    }
    
    if (cursor == console->limit) {
        recycle_screen_buf(console);
        cursor -= NR_CRT_COLUMNS;
    }
    console->cursor = cursor;

    while (console == current_console && screen_detached(console))
        scroll_down(console);

    if (console == current_console) 
        set_cursor(console->start + console->cursor);

    return ch;
}

int putsk(const char *str) 
{
    int ret = 0;
    for(const char *i = str; *i != '\0'; i++) {
        ret = fputchark(current_console, *i);
        if (ret != *i) {
            return ret;
        }
    }
    
    return 0;
}

static int vprintk(struct console *console, const char *fmt, __builtin_va_list args) 
{
    int intarg, len, rem;
    const char *strarg;
    char *pc = console->print_buf;
    for (const char *i = fmt; *i != '\0'; i++) {
        if (*i == '%') {
            int found = 1;
            switch (*(i+1)) {
            case 'c':
                intarg = __builtin_va_arg(args, int);
                *pc++ = intarg;
                break;
            case 'd':
                intarg = __builtin_va_arg(args, int);
                if (intarg & 0x8000) {
                    intarg = ~intarg + 1;
                    *(pc++) = '-';
                }
                len = number_strlen(intarg, 10);
                for (int j = len; j > 0; j--) {
                    rem = intarg % 10;
                    *(pc+j-1) = rem + '0';
                    intarg /= 10;
                }
                pc += len;
                break;
            case 'x':
                intarg = __builtin_va_arg(args, int);
                len = number_strlen(intarg, 16);
                for (int j = len; j > 0; j--) {
                    rem = intarg % 16;
                    *(pc+j-1) = rem > 9 ? (rem - 10 + 'a') : (rem + '0');
                    intarg /= 16;
                }
                pc += len;
                break;
            case 's':
                strarg = __builtin_va_arg(args, const char*);
                while (*strarg) {
                    *pc++ = *strarg++;
                }                
                break;
            default:
                found = 0;
                break;
            }

            if (found) {
                i++;
                continue;
            }        
        }

        *pc++ = *i;        
    }

    for (char *i = console->print_buf; i < pc; i++) {
        fputchark(console, *i);
    }
    return pc - console->print_buf;
}

int printk(const char *fmt, ...) 
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int n = vprintk(current->tty->console, fmt, args);
    __builtin_va_end(args);
    return n;
}

int fprintk(struct console *console, const char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int n = vprintk(console, fmt, args);
    __builtin_va_end(args);
    return n;
}