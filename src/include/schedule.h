#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include "proc.h"

extern t_proc *current;
t_proc *create_proc(void *text);
t_proc *create_kproc(void *text);
t_proc *next_proc();
void terminate_proc(t_proc *proc);
void destroy_proc(t_proc *proc);

#endif