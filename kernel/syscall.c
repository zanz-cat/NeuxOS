#include <include/syscall.h>

#include "syscall.h"

void *syscall_handler_table[SYSCALL_MAX];

void syscall_register(int irq, void *handler)
{
    syscall_handler_table[irq] = handler;
}