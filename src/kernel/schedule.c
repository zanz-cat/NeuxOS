#include "const.h"
#include "stdio.h"
#include "protect.h"
#include "proc.h"
#include "gdt.h"
#include "log.h"

#define MAX_PROC_NUM    10
#define INITIAL_EFLAGS  0x200

#define SELECTOR_TASK_CS    0x7
#define SELECTOR_TASK_DS    0xf
#define SELECTOR_TASK_SS    0xf
#define SELECTOR_TASK_SS0   0x14

/* the first process is cpu idle process 
 * which keeps cpu running while no task to run
 */
static t_proc proc_list[MAX_PROC_NUM+1];
static int proc_num = 1;
t_proc *current = &proc_list[1];

static int init_proc(t_proc *proc, u32 pid, void *text) {
    int ret = 0;
    proc->pid = pid;
    proc->text = (u32)text;
    proc->ss = SELECTOR_TASK_SS;
    proc->esp = (u32)proc + sizeof(*proc);

    /* eflags */
    proc->esp -= 4;
    *((u32*)proc->esp) = INITIAL_EFLAGS;
    /* cs */
    proc->esp -= 4;
    *((u32*)proc->esp) = SELECTOR_TASK_CS;
    /* eip */
    proc->esp -= 4;
    *((u32*)proc->esp) = proc->text;
    /* general registers */
    proc->esp -= 32;
    *((u32*)proc->esp + 3*4) = proc->esp+32; // original esp
    /* ds */
    proc->esp -= 4;
    *((u32*)proc->esp) = SELECTOR_TASK_DS;
    /* es */
    proc->esp -= 4;
    *((u32*)proc->esp) = SELECTOR_TASK_DS;
    /* fs */
    proc->esp -= 4;
    *((u32*)proc->esp) = 0;
    /* gs */
    proc->esp -= 4;
    *((u32*)proc->esp) = SELECTOR_VIDEO;

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

    i = SELECTOR_TASK_SS0 >> 3;
    proc->ldt[i].base_low = 0;
    proc->ldt[i].base_mid = 0;
    proc->ldt[i].base_high = 0;
    proc->ldt[i].limit_low = 0xffff;
    proc->ldt[i].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    proc->ldt[i].attr1 = DA_DRW | DA_DPL0;

    /* install ldt */
    ret = install_ldt(proc->ldt, LDT_SIZE);
    if (ret < 0) {
        printf("install ldt error, pid: %d, errno: %d\n", proc->pid, ret);
        return ret;
    }
    proc->ldt_selector = ret;
    printf("proc pid: %d, ldt selector: 0x%x\n", proc->pid, proc->ldt_selector);

    /* init tss */
    proc->tss.prev = 0;
    proc->tss.ss0 = SELECTOR_TASK_SS0;
    proc->tss.esp0 = proc_stack0_top(proc);
    proc->tss.ldt = proc->ldt_selector;

    /* install tss */
    ret = install_tss(&proc->tss);
    if (ret < 0) {
        printf("install tss error, pid: %d, errno: %d\n", proc->pid, ret);
        return ret;
    }
    proc->tss_selector = ret;
    printf("proc pid: %d, tss selector: 0x%x\n", proc->pid, proc->tss_selector);

    return 0;
}

t_proc *create_proc(void *text) {
    if (proc_num == MAX_PROC_NUM) {
        printf("max process number(%d) exceed!\n", MAX_PROC_NUM);
        return NULL;
    }

    t_proc *proc = &proc_list[proc_num];
    int ret = init_proc(proc, proc_num, text);
    if (ret < 0) {
        printf("init proc error, pid: %d, errno: %d\n", proc->pid, ret);
        return NULL;
    }
    proc_num++;
    log_info("process created, pid: %d\n", proc->pid);
    return proc;
}

t_proc *next_proc() {
    static int pos = -1;

    if (!proc_num) {
        current = NULL;
        return current;
    }

    pos++;
    pos %= proc_num;
    current = &proc_list[pos];
    return current;
}
