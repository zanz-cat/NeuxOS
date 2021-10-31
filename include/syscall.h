#ifndef __INCLUDE_SYSCALL_H__
#define __INCLUDE_SYSCALL_H__

#include <stdint.h>

#define IRQ_EX_SYSCALL 0x70
#define SYSCALL_EXIT 0
#define SYSCALL_READ 1
#define SYSCALL_WRITE 2
#define SYSCALL_DELAY 3

struct syscall_request {
    uint32_t irq;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
};

#endif