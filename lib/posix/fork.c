#include <errno.h>
#include <syscall.h>

#include <unistd.h>

pid_t fork(void)
{
    pid_t ret = 0;

    asm("movl %1, %%eax\n\t"
        "int %2\n\t"
        :"+a"(ret)
        :"i"(SYSCALL_CLONE), "i"(IRQ_EX_SYSCALL)
        :);
    if (ret < 0) {
        errno = ret;
        return -1;
    }
    return ret;
}