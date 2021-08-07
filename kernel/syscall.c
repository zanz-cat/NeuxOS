#include <include/syscall.h>

#include "kernel.h"
#include "clock.h"
#include "proc.h"
#include "tty.h"

typedef void* syscall_handler;

static int sys_get_ticks()
{
    return kget_jeffies();
}

static int sys_write(char *buf, int len, struct process *proc)
{
    int i;
    for (i = 0; i < len; i++) {
        if (tty_putchar(proc->tty, buf[i]) < 0) {
            break;
        }
    }
    return i;
}

syscall_handler syscall_handler_table[] = {
    [SYSCALL_GET_TICKS] = sys_get_ticks,
    [SYSCALL_WRITE] = sys_write,
};