#include <string.h>

size_t strlen(const char *s)
{
    size_t i;
    for (i = 0; s[i] != '\0'; i++);
    return i;
}

int strcmp(const char *s1, const char *s2)
{
    if (s1 == s2) {
        return 0;
    }
    while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    if (s1 == s2) {
        return 0;
    }
    while (*s1 != '\0' && *s2 != '\0' && n != 1 && *s1 == *s2) {
        n--;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}