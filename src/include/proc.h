#ifndef __PROC_H__
#define __PROC_H__

#include "type.h"

#define DEFAULT_STACK_SIZE 1024

typedef struct s_proc {
    u32 pid;
    u32 text;
    u32 ss;
    u32 esp;
    u8  stack[DEFAULT_STACK_SIZE];
} t_proc;

#endif