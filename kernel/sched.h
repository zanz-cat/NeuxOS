#ifndef __KERNEL_SCHED_H__
#define __KERNEL_SCHED_H__

#include <arch/x86.h>
#include "task.h"

extern struct task *current;

void sched_setup(void);

struct task *create_user_task(void *text, const char *exe, int tty);
struct task *create_kernel_task(void *text, const char *exe, int tty);
void term_task(uint32_t pid);

void sched_task(void);
void yield(void);
void suspend_task(struct list_node *wakeup_queue);
void resume_task(struct list_node *wakeup_queue);
void sched_report(void);

#endif