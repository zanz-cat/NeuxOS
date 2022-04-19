#include <errno.h>
#include <syscall.h>
#include <neuxos.h>
#include <string.h>
#include <stringex.h>

int abs(int n)
{
    return n < 0 ? -n : n;
}

void exit(int status)
{
    asm("movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "int %2\n\t"
        :
        :"i"(SYSCALL_EXIT), "m"(status), "i"(IRQ_EX_SYSCALL)
        :"%eax", "%ebx");
    while (1); // never reach
}

char *realpath(const char *path, char *resolved_path)
{
    char *p;
    char buf[MAX_PATH_LEN];

    if (strlen(path) >= MAX_PATH_LEN) {
        errno = -ENAMETOOLONG;
        return NULL;
    }

    if (resolved_path == NULL) {
        // resolved_path = malloc(MAX_PATH_LEN);
        errno = -EINVAL;
        return NULL;
    }

    p = buf;
    strcpy(buf, path);
    p = trim(p);

    return resolved_path;
}