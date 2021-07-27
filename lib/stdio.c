#include <kernel/sched.h>
#include <lib/unistd.h>
#include <lib/common.h>
#include <lib/stdio.h>

int vsprintf(char *buf, const char *fmt, __builtin_va_list va_arg)
{
    int intarg, len, rem, found, j;
    const char *strarg, *i;
    char *pc = buf;

    for (i = fmt; *i != '\0'; i++) {
        if (*i == '%') {
            found = 1;
            switch (*(i+1)) {
                case 'c':
                    intarg = __builtin_va_arg(va_arg, int);
                    *pc++ = intarg;
                    break;
                case 'd':
                    intarg = __builtin_va_arg(va_arg, int);
                    if (intarg & 0x80000000) {
                        intarg = ~intarg + 1;
                        *(pc++) = '-';
                    }
                    len = number_strlen(intarg, 10);
                    for (j = len; j > 0; j--) {
                        rem = intarg % 10;
                        *(pc+j-1) = rem + '0';
                        intarg /= 10;
                    }
                    pc += len;
                    break;
                case 'x':
                    intarg = __builtin_va_arg(va_arg, int);
                    len = number_strlen(intarg, 16);
                    for (j = len; j > 0; j--) {
                        rem = intarg % 16;
                        *(pc+j-1) = rem > 9 ? (rem - 10 + 'a') : (rem + '0');
                        intarg /= 16;
                    }
                    pc += len;
                    break;
                case 's':
                    strarg = __builtin_va_arg(va_arg, const char*);
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

    return pc - buf;
}

int printf(const char *fmt, ...) 
{
    int len;
    char buf[PRINTF_BUF_SIZE];
    __builtin_va_list args;

    __builtin_va_start(args, fmt);
    len = vsprintf(buf, fmt, args);
    __builtin_va_end(args);
    
    if (len < 0) {
        return len;
    }
    return write(buf, len);
}

int sprintf(char *buf, const char *fmt, ...) 
{
    int len;
    __builtin_va_list args;

    __builtin_va_start(args, fmt);
    len = vsprintf(buf, fmt, args);
    __builtin_va_end(args);
    return len;
}