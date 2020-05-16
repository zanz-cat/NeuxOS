#ifndef __PROC_H__
#define __PROC_H__

#include "type.h"

#define DEFAULT_STACK_SIZE 1024

typedef struct s_proc {
    int pid;
    void *text;
    u8  stack[DEFAULT_STACK_SIZE];
} t_proc;

#endif