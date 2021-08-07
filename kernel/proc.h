#ifndef __PROC_H__
#define __PROC_H__

#include <stdint.h>

#include "proc.h"
#include "protect.h"
#include "tty.h"

#define LDT_SIZE     128
#define STACK0_SIZE  10240
#define STACK3_SIZE  10240

#define PROC_STATE_INIT         0
#define PROC_STATE_RUNNING      1
#define PROC_STATE_TERM         2
#define PROC_STATE_WAIT         3

#define PROC_TYPE_KERNEL    0
#define PROC_TYPE_USER      1

/* user process stack frame */
struct user_stackframe {
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t _esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp3;
    uint32_t ss3;
};

/* kernel process stack frame */
struct kern_stackframe {
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t _esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
};

struct process {
    uint32_t pid;
    uint8_t  type;
    uint8_t  state;
    uint16_t ss0;
    uint32_t esp0;
    uint32_t ticks;
    int tty;
    uint16_t ldt_sel;
    struct descriptor ldt[LDT_SIZE];
    uint8_t  stack0[STACK0_SIZE];
    uint8_t  stack3[STACK3_SIZE];
};

#define proc_user_stack(p) ((uint32_t)(p) + sizeof(struct process))
#define proc_kernel_stack(p) ((uint32_t)((p)->stack3))

#endif