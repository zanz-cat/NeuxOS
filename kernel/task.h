#ifndef __KERNEL_TASK_H__
#define __KERNEL_TASK_H__

#include <stdint.h>

#include <neuxos.h>
#include <arch/x86.h>
#include <lib/list.h>

#include "tty.h"

#define LDT_SIZE 2
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
    uint16_t tss_sel; //place selector here, we can switch task like 'jmp far [task]'
    uint8_t type;
    uint8_t state;
    uint64_t ticks;
    int tty;
    char exe[MAX_PATH_LEN];
    uint32_t text;
    struct tss tss;
    struct list_node list;
    struct list_node running;
} __attribute__((packed));

struct task *create_kernel_task(uint32_t text, const char *exe, int tty);
struct task *create_user_task(const char *exe, int tty);
int destroy_task(struct task *task);

#endif