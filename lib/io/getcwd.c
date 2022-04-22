#include <errno.h>

#include <syscall.h>

#include <unistd.h>

char *getcwd(char *buf, size_t size)
{
    int ret = 0;

    asm("movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "movl %3, %%ecx\n\t"
        "int %4\n\t"
        :"+a"(ret)
        :"i"(SYSCALL_GETCWD), "p"(buf), "m"(size), "i"(IRQ_EX_SYSCALL)
        :"%ebx", "%ecx");
    if (ret < 0) {
        errno = ret;
        return NULL;
    }
    return buf;
}