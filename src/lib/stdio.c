#include "tty.h"
#include "string.h"

static void recycle_screen_buf(struct console *c)
{
    void *src = console_mem_addr(c, NR_CRT_COLUMNS);
    void *dst = console_mem_addr(c, 0);
    memcpy(dst, src, 2*(c->limit - NR_CRT_COLUMNS));
    for (int i = 0; i < NR_CRT_COLUMNS; i++) {
        u16 *a = (u16*)console_mem_addr(c, c->limit - NR_CRT_COLUMNS + i);
        *a = (DEFAULT_TEXT_COLOR << 8);
    }
    c->cursor -= NR_CRT_COLUMNS;
}

u32 _putchar(struct console *console, u32 ch) 
{
    u16 *ptr;
    u32 tmp;
    switch (ch)
    {
    case '\n':
        console->cursor += NR_CRT_COLUMNS - console->cursor % NR_CRT_COLUMNS;
        break;
    case '\b':
        if (console->cursor % NR_CRT_COLUMNS) {
            console->cursor--;
            ptr = (u16*)console_mem_addr(console, console->cursor);
            *ptr = (DEFAULT_TEXT_COLOR << 8);
        }
        break;
    default:
        ptr = (u16*)console_mem_addr(console, console->cursor);
        *ptr = (console->color << 8) | (ch & 0xff);
        console->cursor++;
        break;
    }
    
    if (screen_buf_full(console)) {
        recycle_screen_buf(console);
    }

    while (screen_detached(console)) {
        scroll_down(console);
    }

    if (console == current_console)
        set_cursor(console->start + console->cursor);

    return ch;
}

int backspace() 
{
    return _putchar(current_console, '\b');
}

int putchar(int ch) 
{
    return _putchar(current_console, ch);
}

int puts(const char *str) 
{
    int ret = 0;
    for(const char *i = str; *i != '\0'; i++) {
        ret = _putchar(current_console, *i);
        if (ret != *i) {
            return ret;
        }
    }
    
    return 0;
}

static int _num_strlen(int num, u8 radix) 
{
    int len = 0;
    do {
        len++;
    } while (num /= radix);
    return len;
}

int vprintf(struct console *console, const char *fmt, __builtin_va_list args) 
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
                len = _num_strlen(intarg, 10);
                for (int j = len; j > 0; j--) {
                    rem = intarg % 10;
                    *(pc+j-1) = rem + '0';
                    intarg /= 10;
                }
                pc += len;
                break;
            case 'x':
                intarg = __builtin_va_arg(args, int);
                len = _num_strlen(intarg, 16);
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
        _putchar(console, *i);
    }
    return pc - console->print_buf;
}

int printf(const char *fmt, ...) 
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int n = vprintf(current_console, fmt, args);
    __builtin_va_end(args);
    return n;
}

int fprintf(struct console *console, const char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int n = vprintf(console, fmt, args);
    __builtin_va_end(args);
    return n;
}