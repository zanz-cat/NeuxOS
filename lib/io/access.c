#include <errno.h>

#include <syscall.h>

#include <unistd.h>

int access(const char *pathname, int mode)
{
    int ret = 0;

    asm("movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "movl %3, %%ecx\n\t"
        "int %4\n\t"
        :"+a"(ret)
        :"i"(SYSCALL_ACCESS), "p"(pathname), "m"(mode), "i"(IRQ_EX_SYSCALL)
        :"%ebx", "%ecx");
    if (ret != 0) {
        errno = ret;
        return -1;
    }
    return 0;
}