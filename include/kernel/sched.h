#ifndef __SCHED_H__
#define __SCHED_H__

#include <kernel/protect.h>
#include <kernel/proc.h>

extern struct process *current;
struct process *create_proc(void *text, int tty);
struct process *create_kproc(void *text, int tty);
void terminate_proc(struct process *proc);
void yield();
void proc_sched();

#endif