#include <stddef.h>
#include <string.h>

#include <lib/log.h>

#include "gdt.h"
#include "task.h"

#include "sched.h"

#define MAX_TASK_NUM 10

/* the first is cpu idle task
 * which keeps cpu running while no task to run
 */
static struct task task_list[MAX_TASK_NUM];
static int task_count;
struct task *current;

static struct task *_create_task(void *text, uint16_t type, int tty)
{
    if (task_count == MAX_TASK_NUM) {
        log_error("max task number(%d) exceed!\n", MAX_TASK_NUM);
        return NULL;
    }

    int ret;
    struct task *task = &task_list[task_count];
    memset(task, 0, sizeof(struct task));
    task->state = TASK_STATE_INIT;
    task->type = type;
    task->pid = task_count;
    task->tty = tty;

    if (TASK_TYPE_KERNEL == type) {
        ret = init_kernel_task(task, text);
    } else {
        ret = init_user_task(task, text);
    }
    if (ret < 0) {
        log_error("init task error, pid: %d, errno: %d\n", task->pid, ret);
        return NULL;
    }
    task_count++;
    task->state = TASK_STATE_RUNNING;
    log_debug("task created, pid: %d, LDT sel: 0x%x(%d)\n",
        task->pid, task->ldt_sel, task->ldt_sel >> 3);

    return task;
}

struct task *create_user_task(void *text, int tty) {
    return _create_task(text, TASK_TYPE_USER, tty);
}

struct task *create_kernel_task(void *text, int tty) {
    return _create_task(text, TASK_TYPE_KERNEL, tty);
}

static struct task *next_task() {
    static int pos = 0;

    if (task_count == 1) {
        pos = 0;
    } else {
        pos %= (task_count - 1);
        pos++;
    }

    return &task_list[pos];
}

void term_task(struct task *task) {
    task->state = TASK_STATE_TERM;
}

void yield() {
    //TODO
}

void task_sched() {
    while (task_count) {
        struct task *task = next_task();
        switch (task->state) {
            case TASK_STATE_INIT:
                break;
            case TASK_STATE_RUNNING:
                current = task;
                return;
            case TASK_STATE_TERM:
                log_debug("destroy task: %d\n", task->pid);
                task_count--;
                uninstall_ldt(task->ldt_sel);
                memset(task, 0, sizeof(struct task));
                break;
            default:
                log_error("unknown state, pid: %d, state: %d\n", task->pid, task->state);
                break;
        }
    }
}
