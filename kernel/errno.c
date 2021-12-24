#include <errno.h>

int *__errno_location(void)
{
    static int __errno;
    return &__errno;
}