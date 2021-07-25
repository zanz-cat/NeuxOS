#ifndef __PROC_H__
#define __PROC_H__

#include <type.h>
#include <kernel/proc.h>
#include <kernel/protect.h>
#include <kernel/tty.h>

#define LDT_SIZE    128
#define STACK0_SIZE  1024
#define STACK3_SIZE  1024

#define PROC_STATE_INIT         0
#define PROC_STATE_RUNNING      1
#define PROC_STATE_TERM         2
#define PROC_STATE_WAIT         3

#define PROC_TYPE_KERNEL    0
#define PROC_TYPE_USER      1

/* user process stack frame */
struct user_stackframe {
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
};

/* kernel process stack frame */
struct kern_stackframe {
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
};

struct process {
    u32 pid;
    u8  type;
    u8  state;
    u16 ss0;
    u32 esp0;
    u32 ticks;
    struct tty* tty;
    u16 ldt_sel;
    struct descriptor ldt[LDT_SIZE];
    u8  stack0[STACK0_SIZE];
    u8  stack3[STACK3_SIZE];
};

#define proc_user_stack(p) ((u32)p + sizeof(*p))
#define proc_kernel_stack(p) ((u32)(p->stack3))

#endif