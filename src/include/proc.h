#ifndef __PROC_H__
#define __PROC_H__

#include "type.h"
#include "proc.h"
#include "protect.h"

#define LDT_SIZE    128
#define STACK0_SIZE 1024
#define STACK_SIZE  1024

typedef struct s_proc {
    u32 pid;
    u32 text;
    u32 ss;
    u32 esp;
    DESCRIPTOR ldt[LDT_SIZE];
    u16 ldt_selector;
    TSS tss;
    u16 tss_selector;
    u8  stack0[STACK0_SIZE];
    u8  stack[STACK_SIZE];
} t_proc;

#define proc_stack0_top(p) ((u32)p->stack)
#define proc_stack_top(p) ((u32)p + sizeof(t_proc))

#endif