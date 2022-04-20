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
#define SYSCALL_FSTAT 7
#define SYSCALL_STAT 8
#define SYSCALL_ACCESS 9
#define SYSCALL_GETCWD 10
#define SYSCALL_CHDIR 11

struct sys_getdents_args {
    int fd;
    char *buf;
    size_t nbytes;
    off_t *basep;
};

#endif