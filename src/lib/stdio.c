#include "type.h"

#define DEFAULT_TEXT_COLOR 0x7
#define DEFAULT_PRINT_BUF_SIZE 1024

u32 _putchar(u32 ch, u8 color);
u32 _putchar_pos(u32 ch, u16 pos, u8 color);
void _backspace();

static u8 text_color = DEFAULT_TEXT_COLOR;
static char print_buf[DEFAULT_PRINT_BUF_SIZE];

int set_text_color(u8 color) {
    text_color = color;
    return text_color;
}

int get_text_color() {
    return text_color;
}

int reset_text_color() {
    text_color = DEFAULT_TEXT_COLOR;
    return text_color;
}

int backspace() {
    _backspace();
    return 0;
}

int putchar(int ch) {
    return _putchar(ch, text_color);
}

int puts(const char *str) {
    int ret = 0;
    for(const char *i = str; *i != '\0'; i++) {
        ret = _putchar(*i, text_color);
        if (ret != *i) {
            return ret;
        }
    }
    
    return 0;
}

int putchar_pos(int ch, u16 pos) {
    return _putchar_pos(ch, pos, text_color);
}

int puts_pos(const char *str, u16 pos) {
    int ret = 0;
    for(const char *i = str; *i != '\0'; i++) {
        ret = _putchar_pos(*i, pos+i-str, text_color);
        if (ret != *i) {
            return ret;
        }
    }
    
    return 0;
}

static int _num_strlen(int num, u8 radix) {
    int len = 0;
    do {
        len++;
    } while (num /= radix);
    return len;
}

int printf(const char *fmt, ...) {
    int intarg, len, rem;
    const char *strarg;
    char *pc = print_buf;
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    for (const char *i = fmt; *i != '\0'; i++) {
        if (*i == '%') {
            int found = 1;
            switch (*(i+1)) {
            case 'd':
                intarg = __builtin_va_arg(ap, int);
                len = _num_strlen(intarg, 10);
                for (int j = len; j > 0; j--) {
                    rem = intarg % 10;
                    *(pc+j-1) = rem + '0';
                    intarg /= 10;
                }
                pc += len;
                break;
            case 'x':
                intarg = __builtin_va_arg(ap, int);
                len = _num_strlen(intarg, 16);
                for (int j = len; j > 0; j--) {
                    rem = intarg % 16;
                    *(pc+j-1) = rem > 9 ? (rem - 10 + 'a') : (rem + '0');
                    intarg /= 16;
                }
                pc += len;
                break;
            case 's':
                strarg = __builtin_va_arg(ap, const char*);
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
    __builtin_va_end(ap);

    for (char *i = print_buf; i < pc; i++) {
        _putchar(*i, text_color);
    }
    return pc - print_buf;
}

int printf_pos(u16 pos, const char *fmt, ...) {
    int intarg, len, rem;
    const char *strarg;
    char *pc = print_buf;
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    for (const char *i = fmt; *i != '\0'; i++) {
        if (*i == '%') {
            int found = 1;
            switch (*(i+1)) {
            case 'd':
                intarg = __builtin_va_arg(ap, int);
                len = _num_strlen(intarg, 10);
                for (int j = len; j > 0; j--) {
                    rem = intarg % 10;
                    *(pc+j-1) = rem + '0';
                    intarg /= 10;
                }
                pc += len;
                break;
            case 'x':
                intarg = __builtin_va_arg(ap, int);
                len = _num_strlen(intarg, 16);
                for (int j = len; j > 0; j--) {
                    rem = intarg % 16;
                    *(pc+j-1) = rem > 9 ? (rem - 10 + 'a') : (rem + '0');
                    intarg /= 16;
                }
                pc += len;
                break;
            case 's':
                strarg = __builtin_va_arg(ap, const char*);
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
    __builtin_va_end(ap);

    for (char *i = print_buf; i < pc; i++) {
        _putchar_pos(*i, pos+i-print_buf, text_color);
    }
    return pc - print_buf;
}