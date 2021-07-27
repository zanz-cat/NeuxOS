#include <kernel/const.h>
#include <kernel/clock.h>
#include <kernel/proc.h>
#include <kernel/printk.h>

typedef void* syscall_handler;

static int sys_get_ticks() 
{
    return kget_ticks();
}

static int sys_write(char *buf, int len, struct process *proc)
{
    return fprintk(proc->tty, buf);
}

syscall_handler syscall_handler_table[] = {
    [SYSCALL_GET_TICKS] = sys_get_ticks,
    [SYSCALL_WRITE] = sys_write,
};