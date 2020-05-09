#include "unistd.h"

void _sleep_sec();
void _sleep_usec();

u32 sleep(u32 sec) {
    for (u32 i = 0; i < sec; i++) {
        _sleep_sec();
    }
    return 0;
}

int usleep(u32 usec) {
    for (u32 i = 0; i < usec; i++) {
        _sleep_usec();
    }
    return 0;
}