#include <stddef.h>
#include <stdint.h>

#include <syscall.h>

#include <unistd.h>

#define HZ 60

static uint32_t _get_ticks()
{
    asm("movl %0, %%eax\n\t"
        "int %1"
        ::"i"(SYSCALL_GET_TICKS), "i"(INT_VECTOR_SYSCALL)
        :"%eax");
}

static uint32_t get_ticks()
{
#include <stdio.h>
    uint32_t t = _get_ticks();
    return t;
}

unsigned int sleep(unsigned int seconds)
{
    uint32_t t = get_ticks();
    while ((get_ticks() - t)/HZ < seconds);
    return 0;
}

int usleep(useconds_t useconds)
{
    int t = get_ticks();
    while ((get_ticks() - t)*1000000/HZ < useconds);
    return 0;
}

ssize_t write(int fd, const void *buf, size_t n)
{
    asm("movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "movl %2, %%ecx\n\t"
        "int %3"
        ::"i"(SYSCALL_WRITE), "p"(buf), "m"(n), "i"(INT_VECTOR_SYSCALL)
        :"%eax", "%ebx", "%ecx");
}