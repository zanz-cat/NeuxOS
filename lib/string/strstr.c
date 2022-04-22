#include <string.h>

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