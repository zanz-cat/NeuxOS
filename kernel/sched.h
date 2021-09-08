#ifndef __KERNEL_SCHED_H__
#define __KERNEL_SCHED_H__

#include <arch/x86.h>
#include "task.h"

extern struct task *current;
struct task *create_user_task(void *text, int tty);
struct task *create_kernel_task(void *text, int tty);
void term_task(uint32_t pid);
void sched_task(void);
void yield(void);
void sched_setup(void);
void system_load_report(void);

#endif