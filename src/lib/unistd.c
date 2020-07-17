#include "type.h"
#include "const.h"

u32 get_ticks() {
    asm("movl %0, %%eax;"
        "int $0x70;"
        ::"i"(SYSCALL_GET_TICKS)
        :"%eax");
}

u32 sleep(u32 sec) {
    int t = get_ticks();
    while ((get_ticks() - t)/HZ < sec);
    return 0;
}

u32 msleep(u32 msec) {
    int t = get_ticks();
    while ((get_ticks() - t)*1000/HZ < msec);
    return 0;
}
