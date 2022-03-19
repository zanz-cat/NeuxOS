#include <unistd.h>
#include <dirent.h>

#include <syscall.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

ssize_t getdirentries(int fd, char *buf, size_t nbytes, off_t *basep)
{
    struct sys_getdents_args args = {
        .fd = fd,
        .buf = buf,
        .nbytes = nbytes,
        .basep = basep,
    };

    asm("movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "int %2\n\t"
        :
        :"i"(SYSCALL_GETDENTS), "p"(&args), "i"(IRQ_EX_SYSCALL)
        :"%eax", "%ebx");
}

#pragma GCC diagnostic pop