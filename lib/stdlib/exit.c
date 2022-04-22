#include <syscall.h>

#include <stdlib.h>

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