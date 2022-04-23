#ifndef __KERNEL_TASK_H__
#define __KERNEL_TASK_H__

#include <stdint.h>

#include <neuxos.h>
#include <arch/x86.h>
#include <fs/fs.h>
#include <lib/list.h>

#include "tty.h"

#define LDT_SIZE 2
#define STACK0_SIZE 10240
#define STACK3_SIZE 10240

#define TASK_STATE_INIT 0
#define TASK_STATE_RUNNING 1
#define TASK_STATE_TERM 2
#define TASK_STATE_WAIT 3

#define TASK_T_KERN 0
#define TASK_T_USER 1

#define EFLAGS_IF 0x200
#define INITIAL_EFLAGS (EFLAGS_IF | 0x3000) /* IF | IOPL=3 */

#define NR_TASK_FILES 256

struct task {
    uint32_t pid;
    uint16_t tss_sel; //place selector here, we can switch task by 'jmp far [task]'
    uint8_t type;
    uint8_t state;
    uint64_t ticks;
    struct file *cwd;
    struct tss tss;
    void *stack0;
    uint32_t delay; // us
    struct file *files[NR_TASK_FILES];
    struct list_head list;
    struct list_head running;
} __attribute__((packed));

struct ktask {
    struct task base;
    char name[MAX_NAME_LEN];
};

struct utask {
    struct task base;
    struct file *exe;
};

static inline struct ktask *kern_task(struct task *task)
{
    return (struct ktask *)task;
}

static inline struct utask *user_task(struct task *task)
{
    return (struct utask *)task;
}

static inline const char *task_name(struct task *task)
{
    if (task->type == TASK_T_KERN) {
        return kern_task(task)->name;
    }
    if (user_task(task)->exe == NULL || user_task(task)->exe->dent == NULL) {
        return "null";
    }
    return user_task(task)->exe->dent->name;
}

struct jmp_stack_frame {
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

struct task *create_kernel_task(const char *exe, void *text);
struct task *create_user_task(const char *exe, struct file *stdin,
                              struct file *stdout, struct file *stderr);
int destroy_task(struct task *task);

int task_getcwd(const struct task *task, char *buf, size_t size);
int task_chdir(struct task *task, const char *path);

#endif