#ifndef __PROC_H__
#define __PROC_H__

#include "type.h"
#include "proc.h"
#include "protect.h"

#define LDT_SIZE    128
#define STACK0_SIZE  1024
#define STACK3_SIZE  1024

#define PROC_STATE_INIT         0
#define PROC_STATE_RUNNING      1
#define PROC_STATE_TERM         2
#define PROC_STATE_WAIT         3

#define PROC_TYPE_KERNEL    0
#define PROC_TYPE_USER      1

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
    u32 esp3;
    u32 ss3;
} user_proc_stack_frame;

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
} kernel_proc_stack_frame;

typedef struct {
    u32 pid;
    u8  type;
    u8  state;
    u16 ss0;
    u32 esp0;
    u32 ticks;
    u16 ldt_sel;
    DESCRIPTOR ldt[LDT_SIZE];
    u8  stack0[STACK0_SIZE];
    u8  stack3[STACK3_SIZE];
} t_proc;

#define proc_user_stack(p) ((u32)p + sizeof(t_proc))
#define proc_kernel_stack(p) ((u32)(p->stack3))

#endif