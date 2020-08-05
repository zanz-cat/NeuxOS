#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include "proc.h"

extern struct process *current;
struct process *create_proc(void *text);
struct process *create_kproc(void *text);
void terminate_proc(struct process *proc);
void yield();
void proc_sched();

#endif