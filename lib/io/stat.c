#include <errno.h>
#include <fcntl.h>
#include <syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/stat.h>

int fstat(int fd, struct stat *st)
{
    int ret = 0;

    asm("movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "movl %3, %%ecx\n\t"
        "int %4\n\t"
        :"+a"(ret)
        :"i"(SYSCALL_FSTAT), "m"(fd), "p"(st), "i"(IRQ_EX_SYSCALL)
        :"%ebx", "%ecx");
    if (ret < 0) {
        errno = ret;
        return -1;
    }
    return ret;
}

int stat(const char *pathname, struct stat *st)
{
    int ret = 0;

    asm("movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "movl %3, %%ecx\n\t"
        "int %4\n\t"
        :"+a"(ret)
        :"i"(SYSCALL_STAT), "p"(pathname), "p"(st), "i"(IRQ_EX_SYSCALL)
        :"%ebx", "%ecx");
    if (ret < 0) {
        errno = ret;
        return -1;
    }
    return ret;
}