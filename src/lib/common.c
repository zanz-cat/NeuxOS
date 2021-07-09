#include <lib/common.h>

int number_strlen(int num, u8 radix) 
{
    int len = 0;
    do {
        len++;
    } while (num /= radix);
    return len;
}