#ifndef __PROC_H__
#define __PROC_H__

#include "type.h"
#include "proc.h"
#include "protect.h"

#define LDT_SIZE    128
#define STACK_SIZE  1024

typedef struct {
    u32 gs;
    u32 fs;
    u32 es;
    u32 ds;
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 _esp;
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;
    u32 eip;
    u32 cs;
    u32 eflags;
    u32 esp;
    u32 ss;
} stack_frame;

typedef struct {
    stack_frame regs;
    u32 pid;
    u32 type;
    DESCRIPTOR ldt[LDT_SIZE];
    u16 ldt_sel;
    u8  stack[STACK_SIZE];
} t_proc;

#define proc_stack_top(p) ((u32)p + sizeof(t_proc))

#endif