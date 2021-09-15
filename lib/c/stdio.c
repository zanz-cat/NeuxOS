#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <misc/misc.h>

#define PRINTF_BUF_SIZE 1024

static int vsprintf_d(int arg, char *pbuf, bool left, bool zero, uint16_t min_len)
{
    int i, t, rem;
    char *p = pbuf;
    uint32_t abs = arg < 0 ? (~arg + 1) : arg;
    uint16_t len = intlen(abs, 10);

    if (left) {
        if (arg < 0) {
            *p++ = '-';
        }
        for (i = len; i > 0; i--) {
            rem = abs % 10;
            *(p+i-1) = rem + '0';
            abs /= 10;
        }
        p += len;
        t = min_len - len - (arg < 0 ? 1 : 0);
        for (i = 0; i < t; i++) {
            *p++ = ' ';
        }
    } else {
        if (arg < 0 && zero) {
            *p++ = '-';
        }
        t = min_len - len - (arg < 0 ? 1 : 0);
        for (i = 0; i < t; i++) {
            *p++ = zero ? '0' : ' ';
        }
        if (arg < 0 && !zero) {
            *p++ = '-';
        }
        for (i = len; i > 0; i--) {
            rem = abs % 10;
            *(p+i-1) = rem + '0';
            abs /= 10;
        }
        p += len;
    }
    return p - pbuf;
}

static int vsprintf_u(uint32_t arg, char *pbuf, bool left, bool zero, uint16_t min_len)
{
    int i, rem;
    char *p = pbuf;
    uint16_t len = intlen(arg, 10);

    if (left) {
        for (i = len; i > 0; i--) {
            rem = arg % 10;
            *(p+i-1) = rem + '0';
            arg /= 10;
        }
        p += len;
        for (i = 0; i < min_len - len; i++) {
            *p++ = ' ';
        }
    } else {
        for (i = 0; i < min_len - len; i++) {
            *p++ = zero ? '0' : ' ';
        }
        for (i = len; i > 0; i--) {
            rem = arg % 10;
            *(p+i-1) = rem + '0';
            arg /= 10;
        }
        p += len;
    }
    return p - pbuf;
}

static int vsprintf_x(uint32_t arg, char *pbuf, bool capital, 
                      bool left, bool zero, uint16_t min_len)
{
    int i, t, rem;
    char *p = pbuf;
    uint16_t len = intlen(arg, 16);

    if (left) {
        for (i = len; i > 0; i--) {
            rem = arg % 16;
            *(p+i-1) = rem > 9 ? (rem - 10 + (capital ? 'A' : 'a')) : (rem + '0');
            arg /= 16;
        }
        p += len;
        t = min_len - len;
        for (i = 0; i < t; i++) {
            *p++ = ' ';
        }
    } else {
        t = min_len - len;
        for (i = 0; i < t; i++) {
            *p++ = zero ? '0' : ' ';
        }
        for (i = len; i > 0; i--) {
            rem = arg % 16;
            *(p+i-1) = rem > 9 ? (rem - 10 + (capital ? 'A' : 'a')) : (rem + '0');
            arg /= 16;
        }
        p += len;
    }
    return p - pbuf;
}

static int vsprintf_p(uint32_t arg, char *pbuf, bool left, bool zero, uint16_t min_len)
{
    int i, t, rem;
    char *p = pbuf;
    uint16_t len = intlen(arg, 16);

    if (left) {
        *p++ = '0';
        *p++ = 'x';
        for (i = len; i > 0; i--) {
            rem = arg % 16;
            *(p+i-1) = rem > 9 ? (rem - 10 + 'a') : (rem + '0');
            arg /= 16;
        }
        p += len;
        t = min_len - len - 2; // strip '0x' len
        for (i = 0; i < t; i++) {
            *p++ = ' ';
        }
    } else {
        if (zero) {
            *p++ = '0';
            *p++ = 'x';
        }
        t = min_len - len - 2;
        for (i = 0; i < t; i++) {
            *p++ = zero ? '0' : ' ';
        }
        if (!zero) {
            *p++ = '0';
            *p++ = 'x'; 
        }
        for (i = len; i > 0; i--) {
            rem = arg % 16;
            *(p+i-1) = rem > 9 ? (rem - 10 + 'a') : (rem + '0');
            arg /= 16;
        }
        p += len;
    }
    return p - pbuf;
}

static int vsprintf_s(const char *arg, char *pbuf, bool left, uint16_t min_len)
{
    int i;
    char *p = pbuf;
    size_t len = strlen(arg);
    if (left) {
        while (*arg) {
            *p++ = *arg++;
        }
        for (i = 0; i < (int)(min_len - len); i++) {
            *p++ = ' ';
        }
    } else {
        for (i = 0; i < (int)(min_len - len); i++) {
            *p++ = ' ';
        }
        while (*arg) {
            *p++ = *arg++;
        }
    }
    return p - pbuf;
}

int vsprintf(char *buf, const char *fmt, va_list ap)
{
    const char *ch;
    bool abort, left, zero;
    uint16_t min_len;
    char *pbuf = buf;

    for (ch = fmt; *ch != '\0'; ch++) {
        if (*ch != '%') {
            *pbuf++ = *ch;
            continue;
        }
        
        left = false;
        zero = false;
        min_len = 0;
        abort = false;
        while (!abort) {
            ch++;
            switch (*ch) {
            case '\0':
                ch--;
                abort = true;
                break;
            case '-':
                left = true;
                break;
            case '0':
                if (min_len == 0 && !left && !zero) {
                    zero = true;
                    break;
                }
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                min_len = min_len * 10 + (*ch - '0');
                break;
            case 'c':
                *pbuf++ = va_arg(ap, int);
                abort = true;
                break;
            case 'd':
                pbuf += vsprintf_d(va_arg(ap, int), pbuf, left, zero, min_len);
                abort = true;
                break;
            case 'u':
                pbuf += vsprintf_u(va_arg(ap, uint32_t), pbuf, left, zero, min_len);
                abort = true;
                break;
            case 'x':
                pbuf += vsprintf_x(va_arg(ap, uint32_t), pbuf, false, left, zero, min_len);
                abort = true;
                break;
            case 'X':
                pbuf += vsprintf_x(va_arg(ap, uint32_t), pbuf, true, left, zero, min_len);
                abort = true;
                break;
            case 's':
                pbuf += vsprintf_s(va_arg(ap, const char*), pbuf, left, min_len);
                abort = true;
                break;
            case 'p':
                pbuf += vsprintf_p(va_arg(ap, uint32_t), pbuf, left, zero, min_len);
                abort = true;
                break;
            default:
                *pbuf++ = *ch;
                break;
            }
        }
    }
    *pbuf = '\0';
    return pbuf - buf;
}

int printf(const char *fmt, ...)
{
    int len;
    char buf[PRINTF_BUF_SIZE];
    va_list args;

    va_start(args, fmt);
    len = vsprintf(buf, fmt, args);
    va_end(args);

    if (len < 0) {
        return len;
    }
    return write(0, buf, len);
}

int sprintf(char *buf, const char *fmt, ...)
{
    int len;
    va_list args;

    va_start(args, fmt);
    len = vsprintf(buf, fmt, args);
    va_end(args);
    return len;
}