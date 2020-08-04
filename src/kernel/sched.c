#include "const.h"
#include "stdio.h"
#include "protect.h"
#include "proc.h"
#include "gdt.h"
#include "log.h"
#include "string.h"

#define MAX_PROC_NUM            10
#define INITIAL_EFLAGS          0x200 | 0x3000

#define SELECTOR_TASK_CS        0x7
#define SELECTOR_TASK_DS        0xf
#define SELECTOR_TASK_SS0       0x14
#define SELECTOR_TASK_SS3       SELECTOR_TASK_DS

#define SELECTOR_KERN_TASK_CS   0x4
#define SELECTOR_KERN_TASK_DS   0xc
#define SELECTOR_KERN_TASK_SS   SELECTOR_KERN_TASK_DS

/* the first process is cpu idle process 
 * which keeps cpu running while no task to run
 */
static t_proc proc_list[MAX_PROC_NUM];
static int proc_num = 0;
t_proc *current = NULL;

static int init_proc(t_proc *proc, u32 pid, void *text) 
{
    proc->pid = pid;
    proc->type = 1;

    /* construct initial kernel stack */
    user_proc_stack_frame* frame = (user_proc_stack_frame*)(proc_kernel_stack(proc) - sizeof(*frame));
    frame->gs = SELECTOR_VIDEO;
    frame->es = SELECTOR_TASK_DS;
    frame->ds = SELECTOR_TASK_DS;
    frame->eip = (u32)text;
    frame->cs = SELECTOR_TASK_CS;
    frame->eflags = INITIAL_EFLAGS;
    frame->esp3 = proc_user_stack(proc);
    frame->ss3 = SELECTOR_TASK_SS3;

    proc->ss0 = SELECTOR_TASK_SS0;
    proc->esp0 = (u32)frame;

    /* init ldt */
    u16 i = SELECTOR_TASK_CS >> 3;
    proc->ldt[i].base_low = 0;
    proc->ldt[i].base_mid = 0;
    proc->ldt[i].base_high = 0;
    proc->ldt[i].limit_low = 0xffff;
    proc->ldt[i].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    proc->ldt[i].attr1 = DA_C | DA_DPL3;

    i = SELECTOR_TASK_DS >> 3;   // for ds, es, ss3
    proc->ldt[i].base_low = 0;
    proc->ldt[i].base_mid = 0;
    proc->ldt[i].base_high = 0;
    proc->ldt[i].limit_low = 0xffff;
    proc->ldt[i].limit_high_attr2 = DA_LIMIT_4K | DA_32 | 0xf;
    proc->ldt[i].attr1 = DA_DRW | DA_DPL3;

    i = SELECTOR_TASK_SS0 >> 3;   // for ss0
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

static int init_kproc(t_proc *proc, u32 pid, void *text) 
{
    proc->pid = pid;
    proc->type = 0;

    /* construct initial kernel stack */
    kernel_proc_stack_frame* frame = (kernel_proc_stack_frame*)(proc_kernel_stack(proc) - sizeof(*frame));
    frame->gs = SELECTOR_VIDEO;
    frame->es = SELECTOR_KERN_TASK_DS;
    frame->ds = SELECTOR_KERN_TASK_DS;
    frame->eip = (u32)text;
    frame->cs = SELECTOR_KERN_TASK_CS;
    frame->eflags = INITIAL_EFLAGS;

    proc->ss0 = SELECTOR_KERN_TASK_DS; // FIXME
    proc->esp0 = (u32)frame;

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

static t_proc *_create_proc(void *text, u16 type) 
{
    if (proc_num == MAX_PROC_NUM) {
        log_error("max proc number(%d) exceed!\n", MAX_PROC_NUM);
        return NULL;
    }

    int ret;
    t_proc *proc = &proc_list[proc_num];
    memset(proc, 0, sizeof(*proc));
    proc->state = PROC_STATE_INIT;
    if (PROC_TYPE_KERNEL == type) {
        ret = init_kproc(proc, proc_num, text);
    } else {
        ret = init_proc(proc, proc_num, text);
    }

    if (ret < 0) {
        log_error("init proc error, pid: %d, errno: %d\n", proc->pid, ret);
        return NULL;
    }
    proc_num++;
    proc->state = PROC_STATE_RUNNING;
    log_debug("proc created, pid: %d, ldt sel: 0x%x(%d)\n", 
        proc->pid, proc->ldt_sel, proc->ldt_sel >> 3);

    return proc;
}

t_proc *create_proc(void *text) {
    return _create_proc(text, PROC_TYPE_USER);
}

t_proc *create_kproc(void *text) {
    return _create_proc(text, PROC_TYPE_KERNEL);
}

t_proc *next_proc() {
    static int pos = 0;

    if (proc_num == 1) {
        pos = 0;
    } else {
        pos %= (proc_num - 1);
        pos++;
    }

    return &proc_list[pos];
}

void terminate_proc(t_proc *proc) {
    proc->state = PROC_STATE_TERM;
}

void yield() {
    //TODO
}

void proc_sched() {
    while (proc_num) {
        t_proc *proc = next_proc();
        switch (proc->state) {
            case PROC_STATE_INIT:
                break;
            case PROC_STATE_RUNNING:
                current = proc;
                return;
            case PROC_STATE_TERM:
                log_debug("destroy proc: %d\n", proc->pid);
                proc_num--;
                uninstall_ldt(proc->ldt_sel);
                memset(proc, 0, sizeof(*proc));
                break;
            default:
                log_error("unknown state, pid: %d, state: %d\n", proc->pid, proc->state);
                break;
        }
    }
}
