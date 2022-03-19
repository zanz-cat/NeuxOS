#include <fcntl.h>
#include <syscall.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

int open(const char *pathname, int flags, ...)
{
    asm("movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "movl %2, %%ecx\n\t"
        "int %3\n\t"
        :
        :"i"(SYSCALL_OPEN), "p"(pathname), "m"(flags), "i"(IRQ_EX_SYSCALL)
        :"%eax", "%ebx", "%ecx");
}

#pragma GCC diagnostic pop