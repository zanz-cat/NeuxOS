#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void user_init(void)
{
    int count = 0;
    while (1) {
    }
}

void _start(void)
{
    user_init();
    exit(0);
}