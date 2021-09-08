#include <stddef.h>
#include <string.h>

#include <lib/log.h>
#include <lib/list.h>
#include <config.h>

#include "gdt.h"
#include "task.h"
#include "clock.h"
#include "printk.h"
#include "kernel.h"

#include "sched.h"

static uint32_t task_id;
static LIST_HEAD(running_list);
static struct task *kernel_main_task;
struct task *current;

struct task *create_user_task(void *text, int tty)
{
    struct task *task = create_task(task_id++, text, TASK_TYPE_USER, tty);
    if (task == NULL) {
        log_error("create task error\n");
        return NULL;
    }
    LIST_ADD_TAIL(&running_list, &task->chain);
    log_debug("u-task created, pid: %d\n", task->pid);
    return task;
}

struct task *create_kernel_task(void *text, int tty)
{
    struct task *task = create_task(task_id++, text, TASK_TYPE_KERNEL, tty);
    if (task == NULL) {
        log_error("create task error\n");
        return NULL;
    }
    LIST_ADD_TAIL(&running_list, &task->chain);
    log_debug("k-task created, pid: %d\n", task->pid);
    return task;
}

static struct task *next_task()
{
    struct list_node *node;

    if (LIST_COUNT(&running_list) == 0) {
        return kernel_main_task;
    }

    if (current == kernel_main_task) {
        node = &running_list;
    } else {
        node = &current->chain;
    }
    for (node = node->next; node == &running_list; node = node->next);
    return container_of(node, struct task, chain);
}

void term_task(uint32_t pid)
{
    struct list_node *node;
    struct task *task;

    LIST_FOREACH(&running_list, node) {
        task = container_of(node, struct task, chain);
        if (task->pid == pid) {
            task->state = TASK_STATE_TERM;
            break;
        }
    }
}

static void do_term_task(struct task *task)
{
    LIST_DEL(&task->chain);
    mm_free_user_paging((struct paging_entry *)task->tss.cr3);
    uninstall_ldt(task->tss.ldt);
    uninstall_tss(task->tss_sel);
    mm_free(task);
}

void yield(void) {
    struct task *onboard = current;
    sched_task();
    if (current == onboard) {
        current = kernel_main_task;
    }
    asm("ljmp *(%0)"::"p"(PTR_SUB(&current->tss_sel, sizeof(uint32_t))):);
}

void sched_task(void)
{
    while (LIST_COUNT(&running_list) > 0) {
        struct task *task = next_task();
        switch (task->state) {
            case TASK_STATE_INIT:
                break;
            case TASK_STATE_RUNNING:
                current = task;
                return;
            case TASK_STATE_TERM:
                log_debug("destroy task: %d\n", task->pid);
                do_term_task(task);
                break;
            default:
                kernel_panic("unknown state, pid: %d, state: %d\n", task->pid, task->state);
                break;
        }
    }
}

void system_load_report(void)
{
    struct list_node *node;
    struct task *task;

    printk("*** System Load Report ***\n");
    printk("total %u idle %u\n", kget_jeffies(), kernel_main_task->ticks);
    LIST_FOREACH(&running_list, node) {
        task = container_of(node, struct task, chain);
        printk("%u %u\n", task->pid, task->ticks);
    }
}

void sched_setup(void)
{
    task_id = 0;

    kernel_main_task = create_task(task_id++, kernel_main, TASK_TYPE_KERNEL, -1);
    if (kernel_main_task == NULL) {
        kernel_panic("create kernel_main task error\n");
    }
    kernel_main_task->tss.eflags &= ~(EFLAGS_IF);
    current = kernel_main_task;
}