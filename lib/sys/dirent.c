#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <syscall.h>

ssize_t getdirentries(int fd, char *buf, size_t nbytes, off_t *basep)
{
    ssize_t ret = 0;
    struct sys_getdents_args args = {
        .fd = fd,
        .buf = buf,
        .nbytes = nbytes,
        .basep = basep,
    };

    asm("movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "int %3\n\t"
        :"+a"(ret)
        :"i"(SYSCALL_GETDENTS), "p"(&args), "i"(IRQ_EX_SYSCALL)
        :"%ebx");
    if (ret < 0) {
        errno = ret;
        return -1;
    }
    return ret;
}