#ifndef __KERNEL_TASK_H__
#define __KERNEL_TASK_H__

#include <stdint.h>

#include "x86.h"
#include "tty.h"

#define LDT_SIZE     16
#define STACK0_SIZE  10240
#define STACK3_SIZE  10240

#define TASK_STATE_INIT         0
#define TASK_STATE_RUNNING      1
#define TASK_STATE_TERM         2
#define TASK_STATE_WAIT         3

#define TASK_TYPE_KERNEL    0
#define TASK_TYPE_USER      1

struct task {
    uint32_t pid;
    uint8_t type;
    uint8_t state;
    uint32_t ticks;
    int tty;
    uint16_t tss_sel;
    uint16_t ldt_sel;
    struct tss tss;
    struct descriptor ldt[LDT_SIZE];
    uint8_t stack0[STACK0_SIZE];
    uint8_t stack3[STACK3_SIZE];
} __attribute__((packed));

#define task_user_stack(p) ((uint32_t)(p) + sizeof(struct task))
#define task_kernel_stack(p) ((uint32_t)((p)->stack3))

int init_kernel_task(struct task *task, void *text);
int init_user_task(struct task *task, void *text);

#endif