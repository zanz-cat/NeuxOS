#include <stddef.h>
#include <string.h>

#include <lib/list.h>
#include <config.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>

#include "log.h"
#include "descriptor.h"
#include "task.h"
#include "clock.h"
#include "printk.h"
#include "kernel.h"
#include "interrupt.h"

#include "sched.h"

static uint32_t task_id;
static LIST_HEAD(task_list);
static LIST_HEAD(running_queue);
static LIST_HEAD(term_queue);
static struct task *kernel_loop_task;

struct task *current;

uint32_t start_task(struct task *task)
{
    task->pid = task_id++;
    log_debug("start task, pid: %d, exe: %s\n", task->pid, task->exe);

    LIST_ADD_TAIL(&task_list, &task->list);
    LIST_ENQUEUE(&running_queue, &task->running);
    return task->pid;
}

void term_task(uint32_t pid)
{
    struct list_node *node;
    struct task *task;

    log_debug("term task, pid: %u\n", pid);
    if (current->pid == pid) {
        current->state = TASK_STATE_TERM;
        LIST_ENQUEUE(&term_queue, &current->running);
        return;
    }

    LIST_FOREACH(&running_queue, node) {
        task = container_of(node, struct task, running);
        if (task->pid != pid) {
            continue;
        }
        task->state = TASK_STATE_TERM;
        LIST_DEL(&task->running);
        LIST_ENQUEUE(&term_queue, &task->running);
        break;
    }
}

static void do_term_task(struct task *task)
{
    LIST_DEL(&task->list);
    destroy_task(task);
}

void yield(void) {
    struct list_node *node;
    static struct task *onboard;

    disable_irq();
    onboard = current;
    node = LIST_DEQUEUE(&running_queue);
    if (node == NULL) {
        current = kernel_loop_task;
    } else {
        current = container_of(node, struct task, running);
    }
    LIST_ENQUEUE(&running_queue, &onboard->running);
    asm("ljmp *(%0)"::"p"(current):);
    enable_irq();
}

void suspend_task(struct list_node *queue)
{
    struct list_node *node;

    current->state = TASK_STATE_WAIT;
    LIST_ENQUEUE(queue, &current->running);

    node = LIST_DEQUEUE(&running_queue);
    if (node == NULL) {
        current = kernel_loop_task;
    } else {
        current = container_of(node, struct task, running);
    }
    asm("ljmp *(%0)"::"p"(current):);
}

// called in interrupt routine
void resume_task(struct list_node *queue)
{
    struct list_node *node;
    struct task *task;

    node = LIST_DEQUEUE(queue);
    if (node == NULL) {
        return;
    }
    task = container_of(node, struct task, running);
    task->state = TASK_STATE_RUNNING;
    LIST_ENQUEUE(&running_queue, &current->running);
    current = task;
}

void sched_task(void)
{
    struct task *task;
    struct list_node *node;

    if (current != kernel_loop_task && current->state == TASK_STATE_RUNNING) {
        LIST_ENQUEUE(&running_queue, &current->running);
    }
    node = LIST_DEQUEUE(&running_queue);
    if (node == NULL) {
        current = kernel_loop_task;
    } else {
        current = container_of(node, struct task, running);
    }

    while (LIST_COUNT(&term_queue) > 0) {
        node = LIST_DEQUEUE(&term_queue);
        task = container_of(node, struct task, running);
        do_term_task(task);
    }
}

void sched_setup(void)
{
    task_id = 0;

    kernel_loop_task = create_kernel_task((uint32_t)kernel_loop, "[kernel]", TASK_TYPE_KERNEL);
    if (kernel_loop_task == NULL) {
        kernel_panic("create kernel_loop task error\n");
    };
    kernel_loop_task->tss.eflags &= ~EFLAGS_IF;
    current = kernel_loop_task;
    start_task(kernel_loop_task);
}

static const char *task_state(uint8_t state)
{
    switch (state) {
        case TASK_STATE_INIT:
            return "I";
        case TASK_STATE_RUNNING:
            return "R";
        case TASK_STATE_TERM:
            return "T";
        case TASK_STATE_WAIT:
            return "S";
        default:
            return "U";
    }
}
void sched_report(void)
{
    struct list_node *node;
    struct task *task;

    printk("pid  state  exe\n");
    LIST_FOREACH(&task_list, node) {
        task = container_of(node, struct task, list);
        printk("%-4d %s      %s\n", task->pid, task_state(task->state), task->exe);
    }
}