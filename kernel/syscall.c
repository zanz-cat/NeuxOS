#include <include/syscall.h>

#include "clock.h"
#include "task.h"
#include "tty.h"

typedef void* syscall_handler;

static int sys_get_ticks()
{
    return kget_jeffies();
}

static int sys_write(char *buf, int len, struct task *task)
{
    int i;
    for (i = 0; i < len; i++) {
        if (tty_putchar(task->tty, buf[i]) < 0) {
            break;
        }
    }
    return i;
}

syscall_handler syscall_handler_table[] = {
    [SYSCALL_GET_TICKS] = sys_get_ticks,
    [SYSCALL_WRITE] = sys_write,
};