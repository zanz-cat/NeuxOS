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
#include "syscall.h"

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

void utask_bootloader(const char *exe)
{
    uint32_t ebp;
    struct inode *pinode;
    struct cpu_int_stack_frame *sf;

    user_task(current)->exe = vfs_open(exe, 0);
    if (user_task(current)->exe == NULL) {
        log_error("open file %s error: %d\n", exe, errno);
        task_term(current);
    }
    pinode = user_task(current)->exe->dent->inode;
    char *buf = kmalloc(pinode->size);
    if (buf == NULL) {
        log_error("malloc failed\n");
        task_term(current);
    }
    int ret = vfs_read(user_task(current)->exe, buf, pinode->size);
    if (ret < 0) {
        log_error("read error: %d\n", ret);
        kfree(buf);
        task_term(current);
    }
    uint32_t entry_point;
    ret = load_elf(buf, 0, &entry_point);
    kfree(buf);
    if (ret < 0) {
        log_error("load elf[%s] error: %s\n", exe, strerror(ret));
        task_term(current);
    }

    asm("movl %%ebp, %0":"=r"(ebp)::);
    sf = (void *)(*(uint32_t *)ebp - sizeof(struct cpu_int_stack_frame));   // allocated by create_utask
    sf->eip = entry_point;
    sf->cs = SELECTOR_USER_CS;
    sf->eflags = INITIAL_EFLAGS;
    sf->esp = CONFIG_KERNEL_VM_OFFSET;  // stack bottom, no limit for user stack size
    sf->ss = SELECTOR_USER_DS;
    asm("leave\n\t"); // mov esp, ebp; pop ebp
    asm("lea -0x14(%%ebp), %%esp":::); // esp = ebp - sizeof(struct cpu_int_stack_frame); exe path will be dropped
    asm("mov %0, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "iret\n\t"::"i"(SELECTOR_USER_DS):);
}

uint32_t task_start(struct task *task)
{
    task->pid = task_id++;
    task->state = TASK_STATE_RUNNING;
    log_debug("[sched] start task, pid: %d, exe: %s\n", task->pid,
              task->type == TASK_T_KERN ? kern_task(task)->name :
              *((char **)(task->tss.esp + sizeof(void *))));

    LIST_ADD(&task_list, &task->list);
    LIST_ENQUEUE(&running_queue, &task->running);
    return task->pid;
}

void task_term(struct task *task)
{
    log_debug("[sched] term task, pid: %u\n", task->pid);
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
    int ret;

    LIST_DEL(&task->list);
    if (destroy_task(task) != 0) {
        log_error("destroy task[%d] error: %d\n", task->pid, errno);
        return;
    }
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

static void sys_exit(int status)
{
    log_info("[%u][%s] return code=[%d]\n", current->pid, task_name(current), status);
    task_term(current);
}

static int sys_getcwd(char *buf, size_t size)
{
    return task_getcwd(current, buf, size);
}

static int sys_chdir(const char *path)
{
    return task_chdir(current, path);
}

static int sys_clone(void)
{
    struct utask *task = clone_utask((struct utask *)current);
    if (task == NULL) {
        return errno;
    }
    return task_start((struct task *)task);
}

void sched_setup(void)
{
    task_id = 0;

    kernel_loop_task = create_ktask("[kernel]", kernel_loop);
    if (kernel_loop_task == NULL) {
        kernel_panic("create kernel_loop task error\n");
    }
    kernel_loop_task->tss.eflags &= ~EFLAGS_IF;
    current = kernel_loop_task;
    task_start(kernel_loop_task);

    syscall_register(SYSCALL_EXIT, sys_exit);
    syscall_register(SYSCALL_GETCWD, sys_getcwd);
    syscall_register(SYSCALL_CHDIR, sys_chdir);
    syscall_register(SYSCALL_CLONE, sys_clone);
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
    struct task *task;

    printk("pid  state  exe\n");
    LIST_ENTRY_FOREACH(&task_list, list, task) {
        printk("%-4d %s      %s\n", task->pid, task_state(task->state), task_name(task));
    }
}