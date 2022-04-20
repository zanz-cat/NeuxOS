#include <errno.h>
#include <syscall.h>
#include <neuxos.h>
#include <string.h>
#include <stringex.h>

#include <stdlib.h>

int abs(int n)
{
    return n < 0 ? -n : n;
}

void exit(int status)
{
    asm("movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "int %2\n\t"
        :
        :"i"(SYSCALL_EXIT), "m"(status), "i"(IRQ_EX_SYSCALL)
        :"%eax", "%ebx");
    while (1); // never reach
}