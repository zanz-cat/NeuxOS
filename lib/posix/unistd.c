#include <stddef.h>
#include <stdint.h>

#include <misc/misc.h>
#include <syscall.h>

#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

ssize_t write(int fd, const void *buf, size_t n)
{
    asm("movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "movl %2, %%ecx\n\t"
        "int %3\n\t"
        :
        :"i"(SYSCALL_WRITE), "p"(buf), "m"(n), "i"(IRQ_EX_SYSCALL)
        :"%eax", "%ebx", "%ecx");
}

ssize_t read(int fd, void *buf, size_t n)
{
    asm("movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "movl %2, %%ecx\n\t"
        "int %3\n\t"
        :
        :"i"(SYSCALL_READ), "p"(buf), "m"(n), "i"(IRQ_EX_SYSCALL)
        :"%eax", "%ebx", "%ecx");
}

int usleep(useconds_t useconds)
{
    asm("movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "int %2\n\t"
        :
        :"i"(SYSCALL_DELAY), "m"(useconds), "i"(IRQ_EX_SYSCALL)
        :"%eax", "%ebx");
}

#pragma GCC diagnostic pop

unsigned int sleep(unsigned int seconds)
{
    return usleep(seconds * MICROSEC);
}