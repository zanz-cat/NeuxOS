#include <string.h>

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