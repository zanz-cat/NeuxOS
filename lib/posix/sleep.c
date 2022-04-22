#include <lib/misc.h>
#include <syscall.h>

#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

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