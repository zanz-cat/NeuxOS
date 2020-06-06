#include "const.h"
#include "stdio.h"
#include "protect.h"
#include "proc.h"
#include "gdt.h"
#include "log.h"
#include "string.h"

#define MAX_PROC_NUM            10
#define INITIAL_EFLAGS          0x200

#define SELECTOR_TASK_CS        0x7
#define SELECTOR_TASK_DS        0xf
#define SELECTOR_TASK_SS        0xf

#define SELECTOR_KERN_TASK_CS   0x4
#define SELECTOR_KERN_TASK_DS   0xc
#define SELECTOR_KERN_TASK_SS   0xc

/* the first process is cpu idle process 
 * which keeps cpu running while no task to run
 */
static t_proc proc_list[MAX_PROC_NUM];
static int proc_num = 0;
t_proc *current = NULL;

static int init_proc(t_proc *proc, u32 pid, void *text) {
    proc->pid = pid;
    proc->type = 1;

    proc->regs.ds = SELECTOR_TASK_DS;
    proc->regs.es = SELECTOR_TASK_DS;
    proc->regs.gs = SELECTOR_VIDEO;
    proc->regs.eip = (u32)text;
    proc->regs.cs = SELECTOR_TASK_CS;
    proc->regs.eflags = INITIAL_EFLAGS;
    proc->regs.esp = proc_stack_top(proc);
    proc->regs.ss = SELECTOR_TASK_SS;


    /* init ldt */
    u16 i = SELECTOR_TASK_CS >> 3;
    proc->ldt[i].base_low = 0;
    proc->ldt[i].base_mid = 0;
    proc->ldt[i].base_high = 0;
    proc->ldt[i].limit_low = 0xffff;
    proc->ldt[i].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    proc->ldt[i].attr1 = DA_C | DA_DPL3;

    i = SELECTOR_TASK_DS >> 3;   // for ds, es, ss
    proc->ldt[i].base_low = 0;
    proc->ldt[i].base_mid = 0;
    proc->ldt[i].base_high = 0;
    proc->ldt[i].limit_low = 0xffff;
    proc->ldt[i].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    proc->ldt[i].attr1 = DA_DRW | DA_DPL3;

    /* install ldt */
    int sel = install_ldt(proc->ldt, LDT_SIZE);
    if (sel < 0) {
        log_error("install ldt error, pid: %d, errno: %d\n", proc->pid, sel);
        return -1;
    }
    proc->ldt_sel = sel;

    return 0;
}

static int init_kproc(t_proc *proc, u32 pid, void *text) {
    proc->pid = pid;
    proc->type = 0;

    proc->regs.esp = proc_stack_top(proc);
    proc->regs.ss = SELECTOR_KERN_TASK_SS;

    /* push eflags */
    proc->regs.esp -= 4;
    *((u32*)proc->regs.esp) = INITIAL_EFLAGS;
    /* push cs */
    proc->regs.esp -= 4;
    *((u32*)proc->regs.esp) = SELECTOR_KERN_TASK_CS;
    /* push eip */
    proc->regs.esp -= 4;
    *((u32*)proc->regs.esp) = (u32)text;
    /* pusha */
    proc->regs.esp -= 8*4;
    memset((void *)proc->regs.esp, 0, 8*4);
    /* ds */
    proc->regs.esp -= 4;
    *((u32*)proc->regs.esp) = SELECTOR_KERN_TASK_DS;
    /* es */
    proc->regs.esp -= 4;
    *((u32*)proc->regs.esp) = SELECTOR_KERN_TASK_DS;
    /* fs */
    proc->regs.esp -= 4;
    *((u32*)proc->regs.esp) = 0;
    /* gs */
    proc->regs.esp -= 4;
    *((u32*)proc->regs.esp) = SELECTOR_VIDEO;

    /* init ldt */
    u16 i = SELECTOR_KERN_TASK_CS >> 3;
    proc->ldt[i].base_low = 0;
    proc->ldt[i].base_mid = 0;
    proc->ldt[i].base_high = 0;
    proc->ldt[i].limit_low = 0xffff;
    proc->ldt[i].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    proc->ldt[i].attr1 = DA_C | DA_DPL0;

    i = SELECTOR_KERN_TASK_DS >> 3;   // for ds, es, ss
    proc->ldt[i].base_low = 0;
    proc->ldt[i].base_mid = 0;
    proc->ldt[i].base_high = 0;
    proc->ldt[i].limit_low = 0xffff;
    proc->ldt[i].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    proc->ldt[i].attr1 = DA_DRW | DA_DPL0;

    /* install ldt */
    int sel = install_ldt(proc->ldt, LDT_SIZE);
    if (sel < 0) {
        log_error("install ldt error, pid: %d, errno: %d\n", proc->pid, sel);
        return -1;
    }
    proc->ldt_sel = sel;

    return 0;
}

static t_proc *_create_proc(void *text, u16 type) {
    if (proc_num == MAX_PROC_NUM) {
        log_error("max process number(%d) exceed!\n", MAX_PROC_NUM);
        return NULL;
    }

    int ret;
    t_proc *proc = &proc_list[proc_num];
    if (0 == type) {
        ret = init_kproc(proc, proc_num, text);
    } else {
        ret = init_proc(proc, proc_num, text);
    }

    if (ret < 0) {
        log_error("init proc error, pid: %d, errno: %d\n", proc->pid, ret);
        return NULL;
    }
    proc_num++;
    log_debug("process created, pid: %d, ldt selector: 0x%x\n", 
        proc->pid, proc->ldt_sel);
    return proc;
}

t_proc *create_proc(void *text) {
    return _create_proc(text, 1);
}

t_proc *create_kproc(void *text) {
    return _create_proc(text, 0);
}

t_proc *next_proc() {
    static int pos = 0;

    if (proc_num == 1) {
        pos = 0;
    } else {
        pos %= (proc_num - 1);
        pos++;
    }

    current = &proc_list[pos];
    return current;
}

void destroy_proc() {
    proc_num--;
}
