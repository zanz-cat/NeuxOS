#include <stddef.h>
#include <string.h>

#include <lib/list.h>
#include <config.h>

#include "mm.h"
#include "log.h"
#include "gdt.h"
#include "task.h"
#include "clock.h"
#include "printk.h"
#include "kernel.h"
#include "kmalloc.h"
#include "interrupt.h"

#include "sched.h"

static uint32_t task_id;
static LIST_HEAD(task_list);
static LIST_HEAD(running_queue);
static LIST_HEAD(term_queue);
static struct task *kernel_loop_task;

struct task *current;

struct task *create_user_task(void *text, const char *exe, int tty)
{
    struct task *task = create_task(task_id++, text, exe, TASK_TYPE_USER, tty);
    if (task == NULL) {
        log_error("create task error\n");
        return NULL;
    }
    LIST_ADD_TAIL(&task_list, &task->list);
    LIST_ENQUEUE(&running_queue, &task->running);
    log_debug("user task created, pid: %d\n", task->pid);
    return task;
}

struct task *create_kernel_task(void *text, const char *exe, int tty)
{
    struct task *task = create_task(task_id++, text, exe, TASK_TYPE_KERNEL, tty);
    if (task == NULL) {
        log_error("create task error\n");
        return NULL;
    }
    LIST_ADD_TAIL(&task_list, &task->list);
    LIST_ENQUEUE(&running_queue, &task->running);
    log_debug("kernel task created, pid: %d\n", task->pid);
    return task;
}

void term_task(uint32_t pid)
{
    struct list_node *node;
    struct task *task;

    if (current->pid == pid) {
        current->state = TASK_STATE_TERM;
        LIST_DEL(&current->running);
        LIST_ENQUEUE(&term_queue, &current->running);
        return;
    }

    LIST_FOREACH(&running_queue, node) {
        task = container_of(node, struct task, running);
        if (task->pid == pid) {
            task->state = TASK_STATE_TERM;
            LIST_DEL(&task->running);
            LIST_ENQUEUE(&term_queue, &task->running);
            break;
        }
    }
}

static void do_term_task(struct task *task)
{
    LIST_DEL(&task->list);
    mm_free_user_paging((struct paging_entry *)task->tss.cr3);
    uninstall_ldt(task->tss.ldt);
    uninstall_tss(task->tss_sel);
    kfree(task);
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
    asm("ljmp *(%0)"::"p"(PTR_SUB(&current->tss_sel, sizeof(uint32_t))):);
    enable_irq();
}

void suspend_task(struct list_node *wakeup_queue)
{
    struct list_node *node;

    current->state = TASK_STATE_WAIT;
    LIST_ENQUEUE(wakeup_queue, &current->running);

    node = LIST_DEQUEUE(&running_queue);
    if (node == NULL) {
        current = kernel_loop_task;
    } else {
        current = container_of(node, struct task, running);
    }
    asm("ljmp *(%0)"::"p"(PTR_SUB(&current->tss_sel, sizeof(uint32_t))):);
}

void resume_task(struct list_node *wakeup_queue)
{
    struct list_node *node;
    struct task *task;

    node = LIST_DEQUEUE(wakeup_queue);
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

    kernel_loop_task = create_task(task_id++, kernel_loop, "[kernel]", TASK_TYPE_KERNEL, -1);
    if (kernel_loop_task == NULL) {
        kernel_panic("create kernel_loop task error\n");
    };
    kernel_loop_task->tss.eflags &= ~EFLAGS_IF;
    current = kernel_loop_task;
}

void sched_report(void)
{
    struct list_node *node;
    struct task *task;

    printk("pid  state  exe\n");
    LIST_FOREACH(&task_list, node) {
        task = container_of(node, struct task, list);
        printk("%-4d %-5d  %s\n", task->pid, task->state, task->exe);
    }
}