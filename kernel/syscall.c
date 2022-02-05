#include <include/syscall.h>

#include "clock.h"
#include "task.h"
#include "tty.h"
#include "sched.h"
#include "log.h"

static void sys_exit(int status)
{
    log_info("[%u][%s] return code=[%d]\n", current->pid, current->exe, status);
    task_term(current);
}

static int sys_write(int fd, const void *buf, size_t nbytes)
{
    return tty_write(current->tty, (char *)buf, nbytes);
}

static int sys_read(int fd, void *buf, size_t nbytes)
{
    return tty_read(fd, buf, nbytes);
}

static void sys_delay(int us)
{
    delay(us);
}

void *syscall_handler_table[] = {
    [SYSCALL_EXIT] = sys_exit,
    [SYSCALL_READ] = sys_read,
    [SYSCALL_WRITE] = sys_write,
    [SYSCALL_DELAY] = sys_delay,
};