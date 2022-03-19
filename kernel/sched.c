#include <stddef.h>
#include <string.h>
#include <elf.h>
#include <errno.h>

#include <lib/list.h>
#include <config.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <fs/file.h>

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

struct task *current = NULL;

static int load_elf(const void *elf, int voffset, uint32_t *entry_point)
{
    int i, count;
    const Elf32_Ehdr *eh;
    Elf32_Phdr *ph;
    Elf32_Shdr *sh;
    Elf32_Addr dest;

    eh = elf;
    if (strncmp((char *)eh->e_ident, ELFMAG, SELFMAG) != 0) {
        return -EINVAL;
    }
    *entry_point = eh->e_entry;

    count = 0;
    for (i = 0; i < eh->e_phnum; i++) {
        ph = (Elf32_Phdr *)(elf + eh->e_phoff + eh->e_phentsize * i);
        dest = ph->p_vaddr + voffset;
        if (ph->p_type == PT_LOAD && ph->p_filesz > 0 && dest >= 0) {
            memcpy((void *)dest, elf + ph->p_offset, ph->p_filesz);
            count++;
        }
    }
    for (i = 0; i < eh->e_shnum; i++) {
        sh = (Elf32_Shdr *)(elf + eh->e_shoff + eh->e_shentsize * i);
        dest = sh->sh_addr + voffset;
        if (sh->sh_type == SHT_NOBITS && dest >= 0) {
            memset((void *)dest, 0, sh->sh_size);
            count++;
        }
    }
    return count;
}

static void user_task_launcher(void)
{
    uint32_t ebp;
    struct stack_content *stack;

    current->f_exe = vfs_open(current->exe, 0);
    if (current->f_exe == NULL) {
        log_error("open file %s error: %d\n", current->exe, errno);
        task_term(current);
    }
    char *buf = kmalloc(F_INO(current->f_exe)->size);
    if (buf == NULL) {
        log_error("malloc failed\n");
        vfs_close(current->f_exe);
        task_term(current);
    }
    int ret = vfs_read(current->f_exe, buf, F_INO(current->f_exe)->size);
    vfs_close(current->f_exe);
    if (ret < 0) {
        log_error("read error: %d\n", ret);
        kfree(buf);
        task_term(current);
    }
    uint32_t entry_point;
    ret = load_elf(buf, 0, &entry_point);
    kfree(buf);
    if (ret < 0) {
        log_error("load elf error: %d\n", ret);
        task_term(current);
    }

    asm("movl %%ebp, %0":"=r"(ebp)::);
    stack = (void *)(*(uint32_t *)ebp - sizeof(struct stack_content));
    stack->ss = SELECTOR_USER_DS;
    stack->esp = CONFIG_KERNEL_VM_OFFSET;
    stack->eflags = INITIAL_EFLAGS;
    stack->cs = SELECTOR_USER_CS;
    stack->eip = entry_point;
    asm("leave\n\t"
        "mov %0, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "iret\n\t"::"i"(SELECTOR_USER_DS):);
}

uint32_t task_start(struct task *task)
{
    task->pid = task_id++;
    if (task->type == TASK_TYPE_USER) {
        task->tss.eip = (uint32_t)user_task_launcher;
    }
    log_debug("start task, pid: %d, exe: %s\n", task->pid, task->exe);

    LIST_ADD(&task_list, &task->list);
    LIST_ENQUEUE(&running_queue, &task->running);
    return task->pid;
}

void task_term(struct task *task)
{
    log_debug("term task, pid: %u\n", task->pid);
    task->state = TASK_STATE_TERM;
    if (task != current) {
        LIST_DEL(&task->running);
    }
    LIST_ENQUEUE(&term_queue, &task->running);
    if (task == current) {
        yield();
    }
}

static void do_term_task(struct task *task)
{
    LIST_DEL(&task->list);
    destroy_task(task);
}

void yield(void) {
    struct list_head *node;
    static struct task *onboard;

    disable_irq();
    onboard = current;
    node = LIST_DEQUEUE(&running_queue);
    if (node == NULL) {
        current = kernel_loop_task;
    } else {
        current = container_of(node, struct task, running);
    }
    if (onboard->state == TASK_STATE_RUNNING) {
        LIST_ENQUEUE(&running_queue, &onboard->running);
    }
    asm("ljmp *(%0)"::"p"(current):);
    enable_irq();
}

void task_suspend(struct list_head *queue)
{
    struct list_head *node;

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
void task_resume(struct list_head *queue)
{
    struct list_head *node;
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

void task_sched(void)
{
    struct task *task;
    struct list_head *node;

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

    kernel_loop_task = create_kernel_task(kernel_loop, "[kernel]", TASK_TYPE_KERNEL);
    if (kernel_loop_task == NULL) {
        kernel_panic("create kernel_loop task error\n");
    }
    kernel_loop_task->tss.eflags &= ~EFLAGS_IF;
    current = kernel_loop_task;
    task_start(kernel_loop_task);
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
    struct list_head *node;
    struct task *task;

    printk("pid  state  exe\n");
    LIST_FOREACH(&task_list, node) {
        task = container_of(node, struct task, list);
        printk("%-4d %s      %s\n", task->pid, task_state(task->state), task->exe);
    }
}