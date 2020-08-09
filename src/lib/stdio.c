#include "sched.h"
#include "unistd.h"
#include "common.h"
#include "stdio.h"

int vprint(struct console *console, const char *fmt, __builtin_va_list args) 
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

    len = pc - console->print_buf;
    write(console->print_buf, len);
    return len;
}

int printf(const char *fmt, ...) 
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int n = vprint(current->tty->console, fmt, args);
    __builtin_va_end(args);
    return n;
}