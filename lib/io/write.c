#include <stddef.h>
#include <errno.h>

#include <syscall.h>

#include <unistd.h>

ssize_t write(int fd, const void *buf, size_t n)
{
    ssize_t ret = 0;

    asm("movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "movl %3, %%ecx\n\t"
        "movl %4, %%edx\n\t"
        "int %5\n\t"
        :"+a"(ret)
        :"i"(SYSCALL_WRITE), "m"(fd), "p"(buf), "m"(n), "i"(IRQ_EX_SYSCALL)
        :"%ebx", "%ecx", "%edx");
    if (ret < 0) {
        errno = ret;
        return -1;
    }
    return ret;
}