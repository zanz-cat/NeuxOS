#ifndef __KERNEL_SYSCALL_H__
#define __KERNEL_SYSCALL_H__

#include <include/syscall.h>

void syscall_register(int irq, void *handler);

#endif