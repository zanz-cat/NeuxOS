#include "type.h"
#include "const.h"
#include "protect.h"
#include "unistd.h"

u32 get_ticks() 
{
    asm("movl %0, %%eax\n\t"
        "int %1"
        ::"i"(SYSCALL_GET_TICKS), "i"(INT_VECTOR_SYSCALL)
        :"%eax");
}

u32 sleep(u32 sec) 
{
    int t = get_ticks();
    while ((get_ticks() - t)/HZ < sec);
    return 0;
}

u32 msleep(u32 msec) 
{
    int t = get_ticks();
    while ((get_ticks() - t)*1000/HZ < msec);
    return 0;
}

void write(char *buf, int len)
{
    asm("movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "movl %2, %%ecx\n\t"
        "int %3"
        ::"i"(SYSCALL_WRITE), "m"(buf), "m"(len), "i"(INT_VECTOR_SYSCALL)
        :"%eax");
}