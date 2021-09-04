#include <string.h>

#include <lib/log.h>

#include "x86.h"
#include "gdt.h"

#include "task.h"

#define INITIAL_EFLAGS (0x200 | 0x3000) /* IF | IOPL=3 */

#define SELECTOR_USER_TASK_CS 0x7
#define SELECTOR_USER_TASK_DS 0xf
#define SELECTOR_USER_TASK_SS0 0x14
#define SELECTOR_USER_TASK_SS3 SELECTOR_USER_TASK_DS

#define SELECTOR_KERN_TASK_CS 0x4
#define SELECTOR_KERN_TASK_DS 0xc
#define SELECTOR_KERN_TASK_SS SELECTOR_KERN_TASK_DS

int init_kernel_task(struct task *task, void *text)
{
    int sel;
    uint16_t index;

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

    /* install ldt */
    sel = install_ldt(task->ldt, LDT_SIZE);
    if (sel < 0) {
        log_error("install LDT error, pid: %d, errno: %d\n", task->pid, sel);
        return -1;
    }
    task->ldt_sel = sel;

    /* init TSS */
    task->tss.prev = 0;
    task->tss.esp0 = 0;
    task->tss.ss0 = 0;
    task->tss.esp1 = 0;
    task->tss.ss1 = 0;
    task->tss.esp2 = 0;
    task->tss.ss2 = 0;
    task->tss.cr3 = 0;
    task->tss.eip = (uint32_t)text;
    task->tss.eflags = INITIAL_EFLAGS;
    task->tss.eax = 0;
    task->tss.ecx = 0;
    task->tss.edx = 0;
    task->tss.ebx = 0;
    task->tss.esp = task_kernel_stack(task);
    task->tss.ebp = task_kernel_stack(task);
    task->tss.esi = 0;
    task->tss.edi = 0;
    task->tss.es = SELECTOR_KERN_TASK_DS;
    task->tss.cs = SELECTOR_KERN_TASK_CS;
    task->tss.ss = SELECTOR_KERN_TASK_SS;
    task->tss.ds = SELECTOR_KERN_TASK_DS;
    task->tss.fs = 0;
    task->tss.gs = SELECTOR_VIDEO;
    task->tss.ldt = (uint16_t)sel;
    task->tss.attrs = 0;
    task->tss.io = 0;

    sel = install_tss(&task->tss);
    if (sel < 0) {
        log_error("install TSS error, pid: %d, errno: %d\n", task->pid, sel);
        return -1;
    }
    task->tss_sel = (uint16_t)sel;
    return 0;
}

int init_user_task(struct task *task, void *text)
{
    uint16_t index;
    int sel;

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

    /* install ldt */
    sel = install_ldt(task->ldt, LDT_SIZE);
    if (sel < 0) {
        log_error("install LDT error, pid: %d, errno: %d\n", task->pid, sel);
        return -1;
    }
    task->ldt_sel = sel;

    /* init TSS */
    task->tss.prev = 0;
    task->tss.esp0 = task_kernel_stack(task);
    task->tss.ss0 = SELECTOR_USER_TASK_SS0;
    task->tss.esp1 = 0;
    task->tss.ss1 = 0;
    task->tss.esp2 = 0;
    task->tss.ss2 = 0;
    task->tss.cr3 = 0;
    task->tss.eip = (uint32_t)text;
    task->tss.eflags = INITIAL_EFLAGS;
    task->tss.eax = 0;
    task->tss.ecx = 0;
    task->tss.edx = 0;
    task->tss.ebx = 0;
    task->tss.esp = task_user_stack(task);
    task->tss.ebp = task_user_stack(task);
    task->tss.esi = 0;
    task->tss.edi = 0;
    task->tss.es = SELECTOR_USER_TASK_DS;
    task->tss.cs = SELECTOR_USER_TASK_CS;
    task->tss.ss = SELECTOR_USER_TASK_SS3;
    task->tss.ds = SELECTOR_USER_TASK_DS;
    task->tss.fs = 0;
    task->tss.gs = SELECTOR_VIDEO;
    task->tss.ldt = (uint16_t)sel;
    task->tss.attrs = 0;
    task->tss.io = 0;

    sel = install_tss(&task->tss);
    if (sel < 0) {
        log_error("install TSS error, pid: %d, errno: %d\n", task->pid, sel);
        return -1;
    }
    task->tss_sel = (uint16_t)sel;

    return 0;
}