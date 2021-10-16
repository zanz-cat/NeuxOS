#include <include/syscall.h>

#include "clock.h"
#include "task.h"
#include "tty.h"
#include "sched.h"

typedef void* syscall_handler;

static int sys_write(char *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (tty_putchar(current->tty, buf[i]) < 0) {
            break;
        }
    }
    return i;
}

static void sys_delay(int us)
{
    delay(us);
}

syscall_handler syscall_handler_table[] = {
    [SYSCALL_WRITE] = sys_write,
    [SYSCALL_DELAY] = sys_delay,
};