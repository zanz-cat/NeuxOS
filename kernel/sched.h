#ifndef __KERNEL_SCHED_H__
#define __KERNEL_SCHED_H__

#include "task.h"

extern struct task *current;

uint32_t start_task(struct task *task);
void term_task(struct task *task);
void sched_task(void);
void yield(void);
void suspend_task(struct list_node *queue);
void resume_task(struct list_node *queue);
void sched_setup(void);
void sched_report(void);

#endif