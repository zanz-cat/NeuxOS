#include <string.h>

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