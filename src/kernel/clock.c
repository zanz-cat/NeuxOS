#include "stdio.h"

void clock_int_handler() {
    static int count;

    count++;
    if (count % 10000 == 0) {
        printf("tick: %d\n", count);
    }
}