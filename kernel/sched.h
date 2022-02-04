#ifndef __KERNEL_SCHED_H__
#define __KERNEL_SCHED_H__

#include "task.h"

extern struct task *current;

uint32_t task_start(struct task *task);
void task_term(struct task *task);
void task_sched(void);
void task_suspend(struct list_head *queue);
void task_resume(struct list_head *queue);
void yield(void);

void sched_setup(void);
void sched_report(void);

#endif