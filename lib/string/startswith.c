#include <string.h>

int startswith(const char *a, const char *b)
{
    return strncmp(a, b, strlen(b)) == 0;
}