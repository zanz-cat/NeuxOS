#include <string.h>
#include <errno.h>

#include <arch/x86.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>

#include "log.h"
#include "descriptor.h"

#include "task.h"

#define INITIAL_EFLAGS (EFLAGS_IF | 0x3000) /* IF | IOPL=3 */

static int init_kernel_task(struct task *task, void *text)
{
    int ret;
    void *s0;

    s0 = kmemalign(sizeof(uint32_t), STACK0_SIZE);
    if (s0 == NULL) {
        return -ENOMEM;
    }
    memset(s0, 0, STACK0_SIZE);

    /* init TSS */
    task->tss.prev = 0;
    task->tss.esp0 = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.ss0 = 0;
    task->tss.esp1 = 0;
    task->tss.ss1 = 0;
    task->tss.esp2 = 0;
    task->tss.ss2 = 0;
    task->tss.cr3 = (uint32_t)CONFIG_KERNEL_PG_ADDR;
    task->tss.eip = (uint32_t)text;
    task->tss.eflags = INITIAL_EFLAGS;
    task->tss.eax = 0;
    task->tss.ecx = 0;
    task->tss.edx = 0;
    task->tss.ebx = 0;
    task->tss.esp = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.ebp = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.esi = 0;
    task->tss.edi = 0;
    task->tss.es = SELECTOR_KERNEL_DS;
    task->tss.cs = SELECTOR_KERNEL_CS;
    task->tss.ss = SELECTOR_KERNEL_DS;
    task->tss.ds = SELECTOR_KERNEL_DS;
    task->tss.fs = 0;
    task->tss.gs = 0;
    task->tss.attrs = 0;
    task->tss.io = 0;

    ret = install_tss(&task->tss, DA_DPL0);
    if (ret < 0) {
        kfree(s0);
        return ret;
    }
    task->tss_sel = (uint16_t)ret;
    return 0;
}

static int init_user_task(struct task *task)
{
    int ret;
    void *s0;

    s0 = kmemalign(sizeof(uint32_t), STACK0_SIZE);
    if (s0 == NULL) {
        return -ENOMEM;
    }
    memset(s0, 0, STACK0_SIZE);

    /* init TSS */
    task->tss.prev = 0;
    task->tss.esp0 = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.ss0 = SELECTOR_KERNEL_DS;
    task->tss.esp1 = 0;
    task->tss.ss1 = 0;
    task->tss.esp2 = 0;
    task->tss.ss2 = 0;
    task->tss.cr3 = (uint32_t)alloc_user_page();
    // task->tss.eip = (uint32_t)text;
    task->tss.eflags = INITIAL_EFLAGS;
    task->tss.eax = 0;
    task->tss.ecx = 0;
    task->tss.edx = 0;
    task->tss.ebx = 0;
    task->tss.ebp = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.esp = task->tss.ebp - sizeof(struct stack_content);
    task->tss.esi = 0;
    task->tss.edi = 0;
    task->tss.es = SELECTOR_KERNEL_DS;
    task->tss.cs = SELECTOR_KERNEL_CS;
    task->tss.ss = SELECTOR_KERNEL_DS;
    task->tss.ds = SELECTOR_KERNEL_DS;
    task->tss.fs = 0;
    task->tss.gs = 0;
    task->tss.attrs = 0;
    task->tss.io = 0;

    ret = install_tss(&task->tss, DA_DPL3);
    if (ret < 0) {
        kfree(s0);
        return ret;
    }
    task->tss_sel = (uint16_t)ret;
    return 0;
}

struct task *create_kernel_task(void *text, const char *exe, int tty)
{
    int ret;
    struct task *task;

    task = (struct task *)kmalloc(sizeof(struct task));
    if (task == NULL) {
        return NULL;
    }
    memset(task, 0, sizeof(struct task));
    task->state = TASK_STATE_INIT;
    task->type = TASK_TYPE_KERNEL;
    task->pid = 0;
    task->tty = tty;
    strncpy(task->exe, exe, MAX_PATH_LEN);

    ret = init_kernel_task(task, text);
    if (ret < 0) {
        return NULL;
    }
    task->state = TASK_STATE_RUNNING;
    return task;
}

struct task *create_user_task(const char *exe, int tty)
{
    int ret;
    struct task *task;

    task = (struct task *)kmalloc(sizeof(struct task));
    if (task == NULL) {
        return NULL;
    }
    memset(task, 0, sizeof(struct task));
    task->state = TASK_STATE_INIT;
    task->type = TASK_TYPE_USER;
    task->pid = 0;
    task->tty = tty;
    strncpy(task->exe, exe, MAX_PATH_LEN);

    ret = init_user_task(task);
    if (ret < 0) {
        return NULL;
    }
    task->state = TASK_STATE_RUNNING;
    return task;
}

int destroy_task(struct task *task)
{
    if (task->type == TASK_TYPE_USER) {
        free_user_page((void *)task->tss.cr3);
    }
    uninstall_tss(task->tss_sel);
    kfree(PTR_SUB((void *)task->tss.esp0, STACK0_SIZE));
    kfree(task);
    return 0;
}