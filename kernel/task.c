#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <arch/x86.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>

#include "log.h"
#include "descriptor.h"

#include "task.h"

struct interrupt_stack_frame {
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
    struct cpu_int_stack_frame cpu;
} __attribute__((packed));

void irq_iret(void);
void utask_bootloader(const char *exe);

static inline bool is_user(const struct task *task)
{
    return task->type == TASK_T_USER;
}

static inline size_t task_size(const struct task *task)
{
    return is_user(task) ? sizeof(struct utask) : sizeof(struct ktask);
}

static int init_tss(struct task *task, uint32_t stack0, void *text)
{
    int ret;

    task->tss.prev = 0;
    task->tss.esp0 = stack0;
    task->tss.ss0 = is_user(task) ? SELECTOR_KERNEL_DS : 0;
    task->tss.esp1 = 0;
    task->tss.ss1 = 0;
    task->tss.esp2 = 0;
    task->tss.ss2 = 0;
    task->tss.cr3 = is_user(task) ? (uint32_t)alloc_page_table() : (uint32_t)CONFIG_KERNEL_PG_ADDR;
    task->tss.eip = is_user(task) ? (uint32_t)utask_bootloader : (uint32_t)text;
    task->tss.eflags = INITIAL_EFLAGS;
    task->tss.eax = 0;
    task->tss.ecx = 0;
    task->tss.edx = 0;
    task->tss.ebx = 0;
    task->tss.ebp = stack0;
    task->tss.esp = task->tss.ebp;
    task->tss.esi = 0;
    task->tss.edi = 0;
    task->tss.es = SELECTOR_KERNEL_DS;
    task->tss.cs = SELECTOR_KERNEL_CS;
    task->tss.ss = SELECTOR_KERNEL_DS;
    task->tss.ds = SELECTOR_KERNEL_DS;
    task->tss.fs = 0;
    task->tss.gs = 0;
    task->tss.attrs = 0;
    task->tss.io = 0;

    ret = install_tss(&task->tss, is_user(task) ? DA_DPL3 : DA_DPL0);
    if (ret < 0) {
        return ret;
    }
    task->tss_sel = (uint16_t)ret;
    return 0;
}

static void *create_task_stack0(void)
{
    void *s0;

    s0 = kmemalign(sizeof(uint32_t), STACK0_SIZE);
    if (s0 == NULL) {
        errno = -ENOMEM;
        return NULL;
    }
    return PTR_ADD(s0, STACK0_SIZE);
}

static struct task *_create_task(void *text, const char *workdir, uint8_t type)
{
    int ret;
    void *s0 = NULL;
    struct task *task = NULL;

    task = (struct task *)kmalloc(task_size(task));
    if (task == NULL) {
        errno = -ENOMEM;
        goto error;
    }
    memset(task, 0, task_size(task));
    task->state = TASK_STATE_INIT;
    task->type = type;
    task->pid = 0;
    task->cwd = vfs_open(workdir, X_OK);
    if (task->cwd == NULL) {
        goto error;
    }

    s0 = create_task_stack0();
    if (s0 == NULL) {
        goto error;
    }

    ret = init_tss(task, (uint32_t)s0, text);
    if (ret < 0) {
        goto error;
    }
    LIST_HEAD_INIT(&task->list);
    LIST_HEAD_INIT(&task->running);
    return task;

error:
    kfree(s0);
    kfree(task);
    return NULL;
}

struct task *create_ktask(const char *exe, void *text)
{
    struct task *task;

    if (strlen(exe) >= MAX_PATH_LEN) {
        errno = -EINVAL;
        return NULL;
    }

    task = _create_task(text, "/", TASK_T_KERN);
    if (task == NULL) {
        return NULL;
    }
    strcpy(kern_task(task)->name, exe);
    return task;
}

struct task *create_utask(const char *exe, struct file *stdin,
                          struct file *stdout, struct file *stderr)
{
    struct task *task;

    task = _create_task(NULL, "/", TASK_T_USER);
    if (task == NULL) {
        return NULL;
    }
    task->tss.esp -= sizeof(struct cpu_int_stack_frame); // stack space reserved for jmp to userspace
    // put exe path into stack
    task->tss.esp -= strlen(exe) + 1; // exe
    strcpy((char *)task->tss.esp, exe);
    task->tss.esp -= sizeof(char *); // exe args for utask_bootloader
    *((uint32_t *)task->tss.esp) = task->tss.esp + sizeof(char *);
    task->tss.esp -= sizeof(void *); // dummy return address

    task->files[STDIN_FILENO] = stdin;
    task->files[STDOUT_FILENO] = stdout;
    task->files[STDERR_FILENO] = stderr == NULL ? stdout : stderr;
    return task;
}

static int setup_task_state(const struct task *task, struct task *cloned)
{
    void *s0;
    const struct interrupt_stack_frame *frame;
    struct cpu_int_stack_frame *jframe;

    s0 = create_task_stack0();
    if (s0 == NULL) {
        return -1;
    }
    frame = PTR_SUB(task->tss.esp0, sizeof(struct interrupt_stack_frame));
    jframe = PTR_SUB(s0, sizeof(struct cpu_int_stack_frame));
    cloned->tss.gs = frame->gs;
    cloned->tss.fs = frame->fs;
    cloned->tss.es = frame->es;
    cloned->tss.ds = frame->ds;
    cloned->tss.edi = frame->edi;
    cloned->tss.esi = frame->esi;
    cloned->tss.ebp = frame->ebp;
    cloned->tss.esp = (uint32_t)jframe;
    cloned->tss.esp0 = (uint32_t)s0;
    cloned->tss.ebx = frame->ebx;
    cloned->tss.edx = frame->edx;
    cloned->tss.ecx = frame->ecx;
    cloned->tss.eax = 0; // pid 0 child process
    cloned->tss.eip = (uint32_t)irq_iret;
    jframe->eip = frame->cpu.eip;
    jframe->cs = frame->cpu.cs;
    jframe->eflags = frame->cpu.eflags;
    jframe->esp = frame->cpu.esp;
    jframe->ss = frame->cpu.ss;
    return 0;
}

struct utask *clone_utask(struct utask *task)
{
    int i, ret;
    struct page_entry *dir = NULL;
    struct utask *cloned = NULL;

    cloned = (struct utask *)kmalloc(sizeof(struct utask));
    if (cloned == NULL) {
        errno = -ENOMEM;
        return NULL;
    }

    *cloned = *task;
    LIST_HEAD_INIT(&cloned->base.list);
    LIST_HEAD_INIT(&cloned->base.running);
    cloned->base.tss.esp0 = 0;

    ret = setup_task_state(&task->base, &cloned->base);
    if (ret != 0) {
        goto error;
    }
    dir = clone_page_table((const struct page_entry *)task->base.tss.cr3);
    if (dir == NULL) {
        goto error;
    }
    cloned->base.tss.cr3 = (uint32_t)dir;

    ret = install_tss(&cloned->base.tss, DA_DPL3);
    if (ret < 0) {
        goto error;
    }
    cloned->base.tss_sel = (uint16_t)ret;
    cloned->base.cwd->rc++;
    cloned->base.ticks = 0;
    cloned->base.delay = 0;
    cloned->base.cwd->rc++;
    for (i = 0; i < NR_TASK_FILES; i++) {
        if (cloned->base.files[i] != NULL) {
            cloned->base.files[i]->rc++;
        }
    }
    cloned->exe->rc++;

    return cloned;

error:
    free_page_table(dir);
    kfree((void *)cloned->base.tss.esp0);
    kfree(cloned);
    return NULL;
}

int destroy_task(struct task *task)
{
    int fd, ret;

    ret = uninstall_tss(task->tss_sel);
    if (ret != 0) {
        errno = ret;
        return ret;
    }
    kfree(PTR_SUB((void *)task->tss.esp0, STACK0_SIZE));
    if (is_user(task)) {
        // free_page_table((struct page_entry *)task->tss.cr3);
        vfs_close(user_task(task)->exe);
    }
    for (fd = 0; fd < NR_TASK_FILES; fd++) {
        if (task->files[fd] != NULL) {
            vfs_close(task->files[fd]);
        }
    }
    vfs_close(task->cwd);
    kfree(task);
    return 0;
}

int task_getcwd(const struct task *task, char *buf, size_t size)
{
    struct dentry *dent = dentry_obtain(task->cwd->dent);
    if (dentry_abspath(dent, buf, size) != 0) {
        dentry_release(dent);
        return errno;
    }
    dentry_release(dent);
    if (strlen(buf) == 0) {
        sprintf(buf, PATH_SEP);
    }
    return 0;
}

int task_chdir(struct task *task, const char *path)
{
    struct file *dir;

    if (strlen(path) >= MAX_PATH_LEN) {
        return -ENAMETOOLONG;
    }
    
    dir = vfs_open(path, X_OK);
    if (dir == NULL) {
        return errno;
    }

    if (!(dir->dent->inode->mode & S_IFDIR)) {
        vfs_close(dir);
        return -ENOTDIR;
    }

    vfs_close(task->cwd);
    task->cwd = dir;
    return 0;
}