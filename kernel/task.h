#ifndef __KERNEL_TASK_H__
#define __KERNEL_TASK_H__

#include <stdint.h>

#include <neuxos.h>
#include <arch/x86.h>
#include <lib/list.h>

#include "tty.h"
#include "mm.h"

#define LDT_SIZE 16
#define STACK0_SIZE 10240
#define STACK3_SIZE 10240

#define TASK_STATE_INIT 0
#define TASK_STATE_RUNNING 1
#define TASK_STATE_TERM 2
#define TASK_STATE_WAIT 3

#define TASK_TYPE_KERNEL 0
#define TASK_TYPE_USER 1

#define EFLAGS_IF 0x200

struct task {
    uint32_t pid;
    uint16_t tss_sel;
    uint8_t type;
    uint8_t state;
    uint64_t ticks;
    int tty;
    char exe[MAX_PATH_LEN];
    struct tss tss;
    struct descriptor ldt[LDT_SIZE];
    uint32_t stack0;
    uint32_t stack3;
    struct list_node chain;
} __attribute__((packed));

struct task *create_task(uint32_t pid, void *text, uint16_t type, int tty);

#endif