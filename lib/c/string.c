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
    if (s1 == s2 || n == 0) {
        return 0;
    }
    while (*s1 != '\0' && *s2 != '\0' && n != 1 && *s1 == *s2) {
        n--;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

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

char *strstr(const char *haystack, const char *needle)
{
    const char *ch;
    size_t len;

    len = strlen(needle);
    for (ch = haystack; *ch != '\0'; ch++) {
        if (strncmp(ch, needle, len) == 0) {
            return (char *)ch;
        }
    }
    return NULL;
}

size_t strcspn(const char *s, const char *r)
{
    const char *s1, *s2;

    for (s1 = s; *s1 != '\0'; s1++) {
        for (s2 = r; *s2 != '\0'; s2++) {
            if (*s1 == *s2) {
                return (s1 - s);
            }
        }
    }
    return (s1 - s);
}

size_t strspn(const char *s, const char *a)
{
    const char *s1, *s2;

    for (s1 = s; *s1 != '\0'; s1++) {
        for (s2 = a; ; s2++) {
            if (*s2 == '\0') {
                return (s1 - s);
            } else if (*s1 == *s2) {
                break;
            } else {
                //continue
            }
        }
    }
    return (s1 - s);
}

char *strpbrk(const char *s, const char *a)
{
    const char *sc1, *sc2;

    for (sc1 = s; *sc1 != '\0'; sc1++) {
        for (sc2 = a; *sc2 != '\0'; sc2++) {
            if (*sc1 == *sc2) {
                return ((char *)sc1);
            }
        }
    }
    return NULL;
}

char *strsep(char **sp, const char *delim)
{
	char *s = *sp;
	char *p;

	p = NULL;
	if (s && *s && (p = strpbrk(s, delim))) {
		*p++ = 0;
	}

	*sp = p;
	return s;
}