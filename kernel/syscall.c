#include <string.h>
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
    log_info("[%u][%s] return code=[%d]\n", current->pid, task_name(current), status);
    task_term(current);
}

static ssize_t sys_write(int fd, const void *buf, size_t nbytes)
{
    struct file *pfile;

    if (fd >= NR_TASK_FILES) {
        return -EINVAL;
    }

    pfile = current->files[fd];
    if (pfile == NULL) {
        return -EBADFD;
    }
    return vfs_write(pfile, buf, nbytes);
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
    struct file *pfile;

    if (args->fd < 0 || args->fd >= NR_TASK_FILES) {
        return -EINVAL;
    }
    pfile = current->files[args->fd];
    if (pfile == NULL) {
        return -ENOENT;
    }
    if (!(pfile->dent->inode->mode & S_IFDIR)) {
        return -ENOTDIR;
    }

    n = 0;
    for (i = 0; i < (args->nbytes / sizeof(struct dirent)); i++) {
        ret = vfs_readdir(pfile, (struct dirent *)args->buf + i);
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

static int sys_fstat(int fd, struct stat *st)
{
    struct file *pfile;
    struct inode *pinode;

    pfile = current->files[fd];
    if (pfile == NULL) {
        return -1;
    }
    pinode = pfile->dent->inode;
    st->st_mode = pinode->mode;
    st->st_ino = pinode->ino;
    st->st_size = pinode->size;
    return 0;
}

static int sys_stat(const char *pathname, struct stat *st)
{
    struct file *pfile;
    struct inode *pinode;

    pfile = vfs_open(pathname, F_OK);
    if (pfile == NULL) {
        return errno;
    }
    pinode = pfile->dent->inode;
    st->st_mode = pinode->mode;
    st->st_ino = pinode->ino;
    st->st_size = pinode->size;
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

static int sys_getcwd(char *buf, size_t size)
{
    return task_getcwd(current, buf, size);
}

static int sys_chdir(const char *path)
{
    return task_chdir(current, path);
}

void *syscall_handler_table[] = {
    [SYSCALL_EXIT] = sys_exit,
    [SYSCALL_READ] = sys_read,
    [SYSCALL_WRITE] = sys_write,
    [SYSCALL_DELAY] = sys_delay,
    [SYSCALL_OPEN] = sys_open,
    [SYSCALL_CLOSE] = sys_close,
    [SYSCALL_GETDENTS] = sys_getdents,
    [SYSCALL_FSTAT] = sys_fstat,
    [SYSCALL_STAT] = sys_stat,
    [SYSCALL_ACCESS] = sys_access,
    [SYSCALL_GETCWD] = sys_getcwd,
    [SYSCALL_CHDIR] = sys_chdir,
};