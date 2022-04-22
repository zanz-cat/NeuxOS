#include <errno.h>

#include <syscall.h>

#include <unistd.h>

int close(int fd)
{
    int ret = 0;

    asm("movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "int %3\n\t"
        :"+a"(ret)
        :"i"(SYSCALL_CLOSE), "m"(fd), "i"(IRQ_EX_SYSCALL)
        :"%ebx");
    if (ret < 0) {
        errno = ret;
        return -1;
    }
    return ret;
}