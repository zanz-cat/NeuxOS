#include <include/syscall.h>

#include "log.h"
#include "syscall.h"

void *syscall_handler_table[SYSCALL_MAX + 1];

void syscall_register(int irq, void *handler)
{
    if (irq > SYSCALL_MAX) {
        log_error("invalid syscall IRQ %d, ignored.\n", irq);
        return;
    }
    syscall_handler_table[irq] = handler;
}