#ifndef __KERNEL_SCHED_H__
#define __KERNEL_SCHED_H__

#include "x86.h"
#include "task.h"

extern struct task *current;
struct task *create_user_task(void *text, int tty);
struct task *create_kernel_task(void *text, int tty);
void term_task(struct task *task);
void task_sched();
void yield();

#endif