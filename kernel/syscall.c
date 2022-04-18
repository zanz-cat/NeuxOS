#include <errno.h>
#include <sys/stat.h>

#include <include/syscall.h>

#include "clock.h"
#include "task.h"
#include "tty.h"
#include "sched.h"
#include "log.h"
#include "fs/fs.h"

static void sys_exit(int status)
{
    log_info("[%u][%s] return code=[%d]\n", current->pid, current->exe, status);
    task_term(current);
}

static ssize_t sys_write(int fd, const void *buf, size_t nbytes)
{
    return tty_write(current->tty, (char *)buf, nbytes);
}

static ssize_t sys_read(int fd, void *buf, size_t nbytes)
{
    return tty_read(fd, buf, nbytes);
}

static void sys_delay(int us)
{
    delay(us);
}

static int sys_open(const char *pathname, int flags)
{
    int i;
    int fd = -1;

    for (i = 0; i < NR_TASK_FILES; i++) {
        if (current->files[i] == NULL) {
            fd = i;
            break;
        }
    }
    if (fd == -1) {
        return -EMFILE;
    }
    current->files[fd] = vfs_open(pathname, flags);
    if (current->files[fd] == NULL) {
        return errno;
    }
    return fd;
}

static int sys_close(int fd)
{
    int ret;

    if (current->files[fd] == NULL) {
        return -1;
    }
    ret = vfs_close(current->files[fd]);
    if (ret == 0) {
        current->files[fd] = NULL;
    }
    return ret;
}

static ssize_t sys_getdents(struct sys_getdents_args *args)
{
    int i, ret;
    ssize_t n;
    struct file *f;

    if (args->fd < 0 || args->fd >= NR_TASK_FILES) {
        return -EINVAL;
    }
    f = current->files[args->fd];
    if (f == NULL) {
        return -ENOENT;
    }
    if (!(F_INO(f)->mode & S_IFDIR)) {
        return -ENOTDIR;
    }

    n = 0;
    for (i = 0; i < (args->nbytes / sizeof(struct dirent)); i++) {
        ret = vfs_readdir(f, (struct dirent *)args->buf + i);
        if (ret == -EOF) {
            return 0;
        }
        if (ret != 0) {
            return ret;
        }
        n += sizeof(struct dirent);
    }
    *args->basep += (off_t)n;
    return n;
}

static int sys_stat(int fd, struct stat *st)
{
    struct file *f;

    f = current->files[fd];
    if (f == NULL) {
        return -1;
    }
    st->st_mode = f->f_mode;
    st->st_ino = F_INO(f)->ino;
    st->st_size = F_INO(f)->size;
    return 0;
}

static int sys_access(const char *pathname, int mode)
{
    struct dentry *dent;

    if (mode != F_OK) {
        return -ENOTSUP;
    }
    dent = vfs_lookup(pathname);
    if (dent == NULL) {
        return -ENOENT;
    }
    dentry_release(dent);
    return 0;
}

void *syscall_handler_table[] = {
    [SYSCALL_EXIT] = sys_exit,
    [SYSCALL_READ] = sys_read,
    [SYSCALL_WRITE] = sys_write,
    [SYSCALL_DELAY] = sys_delay,
    [SYSCALL_OPEN] = sys_open,
    [SYSCALL_CLOSE] = sys_close,
    [SYSCALL_GETDENTS] = sys_getdents,
    [SYSCALL_STAT] = sys_stat,
    [SYSCALL_ACCESS] = sys_access,
};