#include <string.h>

char *trim(char *s)
{
    char *h, *t;

    h = s;
    t = s + strlen(s) - 1;
    for (; *t == ' ' && t >= h; t--) {
        *t = '\0';
    }
    for (; *h == ' ' && h <= t; h++) {
        *h = '\0';
    }
    return h;
}