#include <string.h>

#include <arch/x86.h>

#include "log.h"
#include "gdt.h"
#include "mm.h"
#include "kmalloc.h"

#include "task.h"

#define INITIAL_EFLAGS (EFLAGS_IF | 0x3000) /* IF | IOPL=3 */

#define SELECTOR_USER_TASK_CS 0x7
#define SELECTOR_USER_TASK_DS 0xf
#define SELECTOR_USER_TASK_SS0 0x14
#define SELECTOR_USER_TASK_SS3 SELECTOR_USER_TASK_DS

#define SELECTOR_KERN_TASK_CS 0x4
#define SELECTOR_KERN_TASK_DS 0xc
#define SELECTOR_KERN_TASK_SS SELECTOR_KERN_TASK_DS

static void install_desc(struct descriptor *table, uint16_t index, 
            uint32_t addr, uint32_t limit, uint16_t attr_type)
{
    struct descriptor *pdesc = &table[index];
    pdesc->base_low = (uint32_t)addr & 0xffff;
    pdesc->base_mid = ((uint32_t)addr >> 16) & 0xff;
    pdesc->base_high = (uint32_t)addr >> 24;

    pdesc->limit_low = limit & 0xffff;
    pdesc->limit_high_attr2 = ((attr_type >> 8) & 0xf0) | ((limit >> 16) & 0xf);
    pdesc->attr1 = attr_type & 0xff;
}

static int init_kernel_task(struct task *task, void *text)
{
    int ret;
    void *ptr;

    /* init ldt 3MB ~ 4GB available */
    install_desc(task->ldt, SELECTOR_KERN_TASK_CS >> 3, 0, 0xfffff, 
                 DA_C|DA_DPL0|DA_LIMIT_4K|DA_32);
    // for ds, es, ss
    install_desc(task->ldt, SELECTOR_KERN_TASK_DS >> 3, 0, 0xfffff, 
                 DA_DRW|DA_DPL0|DA_LIMIT_4K|DA_32);

    ptr = kmemalign(sizeof(uint32_t), STACK0_SIZE);
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
    task->tss.cr3 = (uint32_t)CONFIG_KERNEL_PG_ADDR;
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
    task->tss.gs = 0;
    task->tss.attrs = 0;
    task->tss.io = 0;

    /* install ldt */
    ret = install_ldt(task->ldt, LDT_SIZE);
    if (ret < 0) {
        log_error("install LDT error, pid: %d, errno: %d\n", task->pid, ret);
        kfree(ptr);
        return -1;
    }
    task->tss.ldt = (uint16_t)ret;

    ret = install_tss(&task->tss);
    if (ret < 0) {
        log_error("install TSS error, pid: %d, errno: %d\n", task->pid, ret);
        uninstall_ldt(task->tss.ldt);
        kfree(ptr);
        return -1;
    }
    task->tss_sel = (uint16_t)ret;
    return 0;
}

static int init_user_task(struct task *task, void *text)
{
    int ret;
    void *ptr, *ptr3;

    /* init ldt */
    install_desc(task->ldt, SELECTOR_USER_TASK_CS >> 3, 0, 0xfffff, 
                 DA_C|DA_DPL3|DA_LIMIT_4K|DA_32);
    // for ds, es, ss3
    install_desc(task->ldt, SELECTOR_USER_TASK_DS >> 3, 0, 0xfffff, 
                 DA_DRW|DA_DPL3|DA_LIMIT_4K|DA_32);
    // for ss0
    install_desc(task->ldt, SELECTOR_USER_TASK_SS0 >> 3, 0, 0xfffff, 
                 DA_DRW|DA_DPL0|DA_LIMIT_4K|DA_32);

    ptr = kmemalign(sizeof(uint32_t), STACK0_SIZE);
    if (ptr == NULL) {
        log_error("create stack0 error, pid: %d\n", task->pid);
        return -1;
    }
    ptr3 = kmemalign(sizeof(uint32_t), STACK3_SIZE);
    if (ptr3 == NULL) {
        log_error("create stack3 error, pid: %d\n", task->pid);
        kfree(ptr);
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
    task->tss.gs = 0;
    task->tss.attrs = 0;
    task->tss.io = 0;

    /* install ldt */
    ret = install_ldt(task->ldt, LDT_SIZE);
    if (ret < 0) {
        log_error("install LDT error, pid: %d, errno: %d\n", task->pid, ret);
        kfree(ptr3);
        kfree(ptr);
        return -1;
    }
    task->tss.ldt = (uint16_t)ret;

    ret = install_tss(&task->tss);
    if (ret < 0) {
        log_error("install TSS error, pid: %d, errno: %d\n", task->pid, ret);
        uninstall_ldt(task->tss.ldt);
        kfree(ptr3);
        kfree(ptr);
        return -1;
    }
    task->tss_sel = (uint16_t)ret;

    return 0;
}

struct task *create_task(uint32_t pid, void *text, 
                const char *exe, uint16_t type, int tty)
{
    int ret;
    struct task *task;

    task = (struct task *)kmalloc(sizeof(struct task));
    if (task == NULL) {
        log_error("no enough memory\n");
        return NULL;
    }
    memset(task, 0, sizeof(struct task));
    task->state = TASK_STATE_INIT;
    task->type = type;
    task->pid = pid;
    task->tty = tty;
    strncpy(task->exe, exe, MAX_PATH_LEN);

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