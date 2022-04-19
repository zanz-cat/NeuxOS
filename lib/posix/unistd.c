#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include <lib/misc.h>
#include <syscall.h>

#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

int usleep(useconds_t useconds)
{
    asm("movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "int %2\n\t"
        :
        :"i"(SYSCALL_DELAY), "m"(useconds), "i"(IRQ_EX_SYSCALL)
        :"%eax", "%ebx");
}

#pragma GCC diagnostic pop

unsigned int sleep(unsigned int seconds)
{
    return usleep(seconds * MICROSEC);
}

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

ssize_t read(int fd, void *buf, size_t n)
{
    ssize_t ret = 0;

    asm("movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "movl %3, %%ecx\n\t"
        "movl %4, %%edx\n\t"
        "int %5\n\t"
        :"+a"(ret)
        :"i"(SYSCALL_READ), "m"(fd), "p"(buf), "m"(n), "i"(IRQ_EX_SYSCALL)
        :"%ebx", "%ecx", "%edx");
    if (ret < 0) {
        errno = ret;
        return -1;
    }
    return ret;
}

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

int chdir(const char *path)
{
    int ret = 0;

    asm("movl %1, %%eax\n\t"
        "movl %2, %%ebx\n\t"
        "int %3\n\t"
        :"+a"(ret)
        :"i"(SYSCALL_CHDIR), "p"(path), "i"(IRQ_EX_SYSCALL)
        :"%ebx");
    if (ret < 0) {
        errno = ret;
        return -1;
    }
    return 0;
}