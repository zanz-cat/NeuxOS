#include <string.h>

#include <lib/log.h>
#include <arch/x86.h>

#include "gdt.h"
#include "mm.h"

#include "task.h"

#define INITIAL_EFLAGS (EFLAGS_IF | 0x3000) /* IF | IOPL=3 */

#define SELECTOR_USER_TASK_CS 0x7
#define SELECTOR_USER_TASK_DS 0xf
#define SELECTOR_USER_TASK_SS0 0x14
#define SELECTOR_USER_TASK_SS3 SELECTOR_USER_TASK_DS

#define SELECTOR_KERN_TASK_CS 0x4
#define SELECTOR_KERN_TASK_DS 0xc
#define SELECTOR_KERN_TASK_SS SELECTOR_KERN_TASK_DS

static int init_kernel_task(struct task *task, void *text)
{
    int ret;
    uint16_t index;
    void *ptr;

    /* init ldt */
    index = SELECTOR_KERN_TASK_CS >> 3;
    task->ldt[index].base_low = 0;
    task->ldt[index].base_mid = 0;
    task->ldt[index].base_high = 0;
    task->ldt[index].limit_low = 0xffff;
    task->ldt[index].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    task->ldt[index].attr1 = DA_C | DA_DPL0;

    index = SELECTOR_KERN_TASK_DS >> 3;   // for ds, es, ss
    task->ldt[index].base_low = 0;
    task->ldt[index].base_mid = 0;
    task->ldt[index].base_high = 0;
    task->ldt[index].limit_low = 0xffff;
    task->ldt[index].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    task->ldt[index].attr1 = DA_DRW | DA_DPL0;

    ptr = mm_malloc(STACK0_SIZE, sizeof(uint32_t));
    if (ptr == NULL) {
        log_error("create stack0 error, pid: %d\n", task->pid);
        return -1;
    }
    /* init TSS */
    task->tss.prev = 0;
    task->tss.esp0 = 0;
    task->tss.ss0 = 0;
    task->tss.esp1 = 0;
    task->tss.ss1 = 0;
    task->tss.esp2 = 0;
    task->tss.ss2 = 0;
    task->tss.cr3 = (uint32_t)mm_kernel_page_table();
    task->tss.eip = (uint32_t)text;
    task->tss.eflags = INITIAL_EFLAGS;
    task->tss.eax = 0;
    task->tss.ecx = 0;
    task->tss.edx = 0;
    task->tss.ebx = 0;
    task->tss.esp = (uint32_t)PTR_ADD(ptr, STACK0_SIZE);
    task->tss.ebp = (uint32_t)PTR_ADD(ptr, STACK0_SIZE);
    task->tss.esi = 0;
    task->tss.edi = 0;
    task->tss.es = SELECTOR_KERN_TASK_DS;
    task->tss.cs = SELECTOR_KERN_TASK_CS;
    task->tss.ss = SELECTOR_KERN_TASK_SS;
    task->tss.ds = SELECTOR_KERN_TASK_DS;
    task->tss.fs = 0;
    task->tss.gs = SELECTOR_VIDEO;
    task->tss.attrs = 0;
    task->tss.io = 0;

    /* install ldt */
    ret = install_ldt(task->ldt, LDT_SIZE);
    if (ret < 0) {
        log_error("install LDT error, pid: %d, errno: %d\n", task->pid, ret);
        mm_free(ptr);
        return -1;
    }
    task->tss.ldt = (uint16_t)ret;

    ret = install_tss(&task->tss);
    if (ret < 0) {
        log_error("install TSS error, pid: %d, errno: %d\n", task->pid, ret);
        uninstall_ldt(task->tss.ldt);
        mm_free(ptr);
        return -1;
    }
    task->tss_sel = (uint16_t)ret;
    return 0;
}

static int init_user_task(struct task *task, void *text)
{
    int ret;
    uint16_t index;
    void *ptr, *ptr3;

    /* init ldt */
    index = SELECTOR_USER_TASK_CS >> 3;
    task->ldt[index].base_low = 0;
    task->ldt[index].base_mid = 0;
    task->ldt[index].base_high = 0;
    task->ldt[index].limit_low = 0xffff;
    task->ldt[index].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    task->ldt[index].attr1 = DA_C | DA_DPL3;

    index = SELECTOR_USER_TASK_DS >> 3;   // for ds, es, ss3
    task->ldt[index].base_low = 0;
    task->ldt[index].base_mid = 0;
    task->ldt[index].base_high = 0;
    task->ldt[index].limit_low = 0xffff;
    task->ldt[index].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    task->ldt[index].attr1 = DA_DRW | DA_DPL3;

    index = SELECTOR_USER_TASK_SS0 >> 3;   // for ss0
    task->ldt[index].base_low = 0;
    task->ldt[index].base_mid = 0;
    task->ldt[index].base_high = 0;
    task->ldt[index].limit_low = 0xffff;
    task->ldt[index].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    task->ldt[index].attr1 = DA_DRW | DA_DPL0;

    ptr = mm_malloc(STACK0_SIZE, sizeof(uint32_t));
    if (ptr == NULL) {
        log_error("create stack0 error, pid: %d\n", task->pid);
        return -1;
    }
    ptr3 = mm_malloc(STACK3_SIZE, sizeof(uint32_t));
    if (ptr3 == NULL) {
        log_error("create stack3 error, pid: %d\n", task->pid);
        mm_free(ptr);
        return -1;
    }
    /* init TSS */
    task->tss.prev = 0;
    task->tss.esp0 = (uint32_t)PTR_ADD(ptr, STACK0_SIZE);
    task->tss.ss0 = SELECTOR_USER_TASK_SS0;
    task->tss.esp1 = 0;
    task->tss.ss1 = 0;
    task->tss.esp2 = 0;
    task->tss.ss2 = 0;
    task->tss.cr3 = (uint32_t)mm_alloc_user_page_table();
    task->tss.eip = (uint32_t)text;
    task->tss.eflags = INITIAL_EFLAGS;
    task->tss.eax = 0;
    task->tss.ecx = 0;
    task->tss.edx = 0;
    task->tss.ebx = 0;
    task->tss.esp = (uint32_t)PTR_ADD(ptr3, STACK3_SIZE);
    task->tss.ebp = (uint32_t)PTR_ADD(ptr3, STACK3_SIZE);
    task->tss.esi = 0;
    task->tss.edi = 0;
    task->tss.es = SELECTOR_USER_TASK_DS;
    task->tss.cs = SELECTOR_USER_TASK_CS;
    task->tss.ss = SELECTOR_USER_TASK_SS3;
    task->tss.ds = SELECTOR_USER_TASK_DS;
    task->tss.fs = 0;
    task->tss.gs = SELECTOR_VIDEO;
    task->tss.attrs = 0;
    task->tss.io = 0;

    /* install ldt */
    ret = install_ldt(task->ldt, LDT_SIZE);
    if (ret < 0) {
        log_error("install LDT error, pid: %d, errno: %d\n", task->pid, ret);
        mm_free(ptr3);
        mm_free(ptr);
        return -1;
    }
    task->tss.ldt = (uint16_t)ret;

    ret = install_tss(&task->tss);
    if (ret < 0) {
        log_error("install TSS error, pid: %d, errno: %d\n", task->pid, ret);
        uninstall_ldt(task->tss.ldt);
        mm_free(ptr3);
        mm_free(ptr);
        return -1;
    }
    task->tss_sel = (uint16_t)ret;

    return 0;
}

struct task *create_task(uint32_t pid, void *text, uint16_t type, int tty)
{
    int ret;
    struct task *task;

    task = (struct task *)mm_malloc(sizeof(struct task), 0);
    if (task == NULL) {
        log_error("no enough memory\n");
        return NULL;
    }
    memset(task, 0, sizeof(struct task));
    task->state = TASK_STATE_INIT;
    task->type = type;
    task->pid = pid;
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
    task->state = TASK_STATE_RUNNING;
    return task;
}