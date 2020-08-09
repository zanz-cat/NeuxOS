#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include "proc.h"

extern struct process *current;
struct process *create_proc(void *text, struct tty *ptty);
struct process *create_kproc(void *text, struct tty *ptty);
void terminate_proc(struct process *proc);
void yield();
void proc_sched();

#endif