#include <lib/print.h>
#include <kernel/clock.h>
#include <kernel/proc.h>

typedef void* syscall_handler;

static int sys_get_ticks() 
{
    return kget_ticks();
}

static int sys_write(char *buf, int len, struct process *proc)
{
    for (int i = 0; i < len; i++) {
        fputchark(proc->tty->console, buf[i]);
    }
    return 0;
}

syscall_handler syscall_handler_table[] = {
    sys_get_ticks,
    sys_write
};