#include "const.h"
#include "stdio.h"
#include "proc.h"

#define MAX_PROC_NUM 10

t_proc *current = NULL;
static t_proc proc_list[MAX_PROC_NUM];
static int proc_num = 0;
static int pos = 0;

t_proc *create_proc(void *text) {
    if (proc_num == MAX_PROC_NUM) {
        printf("max process number(%d) exceed!\n", MAX_PROC_NUM);
        return NULL;
    }

    t_proc *proc = &proc_list[proc_num++];
    proc->pid = proc_num;
    proc->text = text;
    printf("process(%d) created\n", proc->pid);
    return proc;
}

void next_proc() {
    asm("xchg %bx, %bx");
    pos = (pos == proc_num - 1) ? 0 : (pos+1);
    current = &proc_list[pos];
}
