#include <stddef.h>
#include <stdint.h>

#include <misc/misc.h>
#include <syscall.h>

#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

#define do_syscall(req) \
    asm("movl %0, %%eax\n\t" \
            "int %1\n\t" \
            : \
            :"p"(req), "i"(IRQ_EX_SYSCALL) \
            :"%eax");

ssize_t write(int fd, const void *buf, size_t n)
{
    struct syscall_request req = {
        .irq = SYSCALL_WRITE,
        .arg0 = fd,
        .arg1 = (int)buf,
        .arg2 = (int)n,
    };
    do_syscall(&req);
}

ssize_t read(int fd, void *buf, size_t n)
{
    struct syscall_request req = {
        .irq = SYSCALL_READ,
        .arg0 = fd,
        .arg1 = (int)buf,
        .arg2 = (int)n,
    };
    do_syscall(&req);
}

int usleep(useconds_t useconds)
{
    struct syscall_request req = {
        .irq = SYSCALL_DELAY,
        .arg0 = (int)useconds,
    };
    do_syscall(&req);
}

#pragma GCC diagnostic pop

unsigned int sleep(unsigned int seconds)
{
    return usleep(seconds * MICROSEC);
}