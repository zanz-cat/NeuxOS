#ifndef __INCLUDE_SYSCALL_H__
#define __INCLUDE_SYSCALL_H__

#include <stdint.h>
#include <stdio.h>

#define IRQ_EX_SYSCALL 0x70
#define SYSCALL_EXIT 0
#define SYSCALL_READ 1
#define SYSCALL_WRITE 2
#define SYSCALL_DELAY 3
#define SYSCALL_OPEN 4
#define SYSCALL_CLOSE 5
#define SYSCALL_GETDENTS 6

struct sys_getdents_args {
    int fd;
    char *buf;
    size_t nbytes;
    off_t *basep;
};

#endif