#include "const.h"
#include "stdio.h"
#include "protect.h"
#include "proc.h"

#define MAX_PROC_NUM 10
#define INITIAL_EFLAGS  0x200

/* the first process is cpu idle process 
 * which keeps cpu running while no task to run
 */
static t_proc proc_list[MAX_PROC_NUM+1];
static int proc_num = 1;
t_proc *current = &proc_list[0];

static void init_proc(t_proc *proc, u32 pid, void *text) {
    proc->pid = pid;
    proc->text = (u32)text;
    proc->ss = SELECTOR_FLAT_RW;
    proc->esp = (u32)proc + sizeof(*proc);

    /* eflags */
    proc->esp -= 4;
    *((u32*)proc->esp) = INITIAL_EFLAGS;
    /* cs */
    proc->esp -= 4;
    *((u32*)proc->esp) = SELECTOR_FLAT_C;
    /* eip */
    proc->esp -= 4;
    *((u32*)proc->esp) = proc->text;
    /* general registers */
    proc->esp -= 32;
    *((u32*)proc->esp + 3*4) = proc->esp+32; // original esp
    /* ds */
    proc->esp -= 4;
    *((u32*)proc->esp) = SELECTOR_FLAT_RW;
    /* es */
    proc->esp -= 4;
    *((u32*)proc->esp) = SELECTOR_FLAT_RW;
    /* fs */
    proc->esp -= 4;
    *((u32*)proc->esp) = 0;
    /* gs */
    proc->esp -= 4;
    *((u32*)proc->esp) = SELECTOR_VIDEO;
}

t_proc *create_proc(void *text) {
    if (proc_num == MAX_PROC_NUM) {
        printf("max process number(%d) exceed!\n", MAX_PROC_NUM);
        return NULL;
    }

    t_proc *proc = &proc_list[proc_num++];
    init_proc(proc, proc_num-1, text);
    printf("process created, pid: %d\n", proc->pid);
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
