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

static inline bool is_user(const struct task *task)
{
    return task->type == TASK_T_USER;
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

static struct task *_create_task(void *text, const char *workdir, uint8_t type)
{
    int ret;
    void *s0 = NULL;
    struct task *task = NULL;

    task = (struct task *)kmalloc(is_user(task) ? sizeof(struct utask) : sizeof(struct ktask));
    if (task == NULL) {
        errno = -ENOMEM;
        goto error;
    }
    memset(task, 0, sizeof(struct task));
    task->state = TASK_STATE_INIT;
    task->type = type;
    task->pid = 0;
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
    task->tss.esp -= sizeof(struct jmp_stack_frame); // stack space reserved for jmp to userspace
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

int destroy_task(struct task *task)
{
    int fd;
    uninstall_tss(task->tss_sel);
    kfree(PTR_SUB((void *)task->tss.esp0, STACK0_SIZE));
    if (is_user(task)) {
        free_user_page((void *)task->tss.cr3);
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