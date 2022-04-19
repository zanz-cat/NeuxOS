#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <arch/x86.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>

#include "log.h"
#include "descriptor.h"

#include "task.h"

static inline bool is_user(const struct task *task)
{
    return task->type == TASK_TYPE_USER;
}

static int init_tss(struct task *task, void *s0, void *text)
{
    int ret;

    task->tss.prev = 0;
    task->tss.esp0 = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.ss0 = is_user(task) ? SELECTOR_KERNEL_DS : 0;
    task->tss.esp1 = 0;
    task->tss.ss1 = 0;
    task->tss.esp2 = 0;
    task->tss.ss2 = 0;
    task->tss.cr3 = is_user(task) ? (uint32_t)alloc_user_page() : (uint32_t)CONFIG_KERNEL_PG_ADDR;
    task->tss.eip = (uint32_t)text; // valid for user task
    task->tss.eflags = INITIAL_EFLAGS;
    task->tss.eax = 0;
    task->tss.ecx = 0;
    task->tss.edx = 0;
    task->tss.ebx = 0;
    task->tss.ebp = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.esp = task->tss.ebp;
    if (is_user(task)) {
        task->tss.esp -= sizeof(struct jmp_stack_frame); // stack space reserved for jmp to userspace
    }
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

static struct task *_create_task(const char *exe, void *text,
                                 const char *workdir, int tty, uint8_t type)
{
    int ret;
    void *s0 = NULL;
    struct task *task = NULL;

    if (strlen(exe) >= MAX_PATH_LEN) {
        errno = -EINVAL;
        goto error;
    }

    task = (struct task *)kmalloc(sizeof(struct task));
    if (task == NULL) {
        errno = -ENOMEM;
        goto error;
    }
    memset(task, 0, sizeof(struct task));
    task->state = TASK_STATE_INIT;
    task->type = type;
    task->pid = 0;
    task->tty = tty;
    strcpy(task->exe, exe);
    task->cwd = vfs_open(workdir, X_OK);
    if (task->cwd == NULL) {
        goto error;
    }

    s0 = kmemalign(sizeof(uint32_t), STACK0_SIZE);
    if (s0 == NULL) {
        errno = -ENOMEM;
        goto error;
    }
    memset(s0, 0, STACK0_SIZE);
    task->stack0 = PTR_ADD(s0, STACK0_SIZE);

    ret = init_tss(task, s0, text);
    if (ret < 0) {
        errno = ret;
        goto error;
    }
    return task;

error:
    kfree(s0);
    kfree(task);
    return NULL;
}

struct task *create_kernel_task(void *text, const char *exe, int tty)
{
    return _create_task(exe, text, "/", tty, TASK_TYPE_KERNEL);
}

struct task *create_user_task(const char *exe, int tty)
{
    return _create_task(exe, NULL, "/", tty, TASK_TYPE_USER);
}

int destroy_task(struct task *task)
{
    if (is_user(task)) {
        free_user_page((void *)task->tss.cr3);
    }
    uninstall_tss(task->tss_sel);
    kfree(PTR_SUB((void *)task->tss.esp0, STACK0_SIZE));
    vfs_close(task->f_exe);
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
        buf[0] = PATH_SEP[0];
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

    if (!(F_INO(dir)->mode & S_IFDIR)) {
        vfs_close(dir);
        return -ENOTDIR;
    }

    vfs_close(task->cwd);
    task->cwd = dir;
    return 0;
}