#include <errno.h>
#include <syscall.h>

#include <fcntl.h>

int open(const char *pathname, int flags, ...)
{
    int ret = 0;

    asm("movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "movl %3, %%ecx\n\t"
        "int %4\n\t"
        :"+a"(ret)
        :"i"(SYSCALL_OPEN), "p"(pathname), "m"(flags), "i"(IRQ_EX_SYSCALL)
        :"%ebx", "%ecx");
    if (ret < 0) {
        errno = ret;
        return -1;
    }
    return ret;
}