#include <string.h>

char *strcpy(char *dest, const char *src)
{
    const char *s1;
    char *s2;

    for (s1 = src, s2 = dest; *s1 != '\0'; s1++, s2++) {
        *s2 = *s1;
    }
    *s2 = '\0';
    return dest;
}

char *strncpy(char *dest, const char *src, size_t count)
{
    char *s;

    s = dest;
    for (; count > 0 && *src != '\0'; count--) {
        *dest++ = *src++;
    }
    for (; count > 0; count--) {
        *dest++ = '\0';
    }
    return s;
}