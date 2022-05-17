#include <string.h>
#include <errno.h>
#include <libgen.h>

#include <stringex.h>
#include <neuxos.h>
#include <mm/kmalloc.h>
#include <kernel/sched.h>
#include <kernel/syscall.h>
#include <kernel/log.h>

#include "fs.h"

static struct dentry *rootfs;

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
    struct file *pfile;

    if (fd >= NR_TASK_FILES) {
        return -EINVAL;
    }

    pfile = current->files[fd];
    if (pfile == NULL) {
        return -EBADFD;
    }
    return vfs_read(pfile, buf, nbytes);
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


void vfs_setup(void)
{
    syscall_register(SYSCALL_OPEN, sys_open);
    syscall_register(SYSCALL_READ, sys_read);
    syscall_register(SYSCALL_WRITE, sys_write);
    syscall_register(SYSCALL_CLOSE, sys_close);
    syscall_register(SYSCALL_GETDENTS, sys_getdents);
    syscall_register(SYSCALL_STAT, sys_stat);
    syscall_register(SYSCALL_FSTAT, sys_fstat);
    syscall_register(SYSCALL_ACCESS, sys_access);
}

int vfs_mount(const char *mountpoint, struct mount *mnt)
{
    struct dentry *dent, *copy;
    char buf[MAX_PATH_LEN];
    char *path;

    strcpy(buf, mountpoint);
    path = trim(buf);
    if (strcmp(path, PATH_SEP) == 0) {
        if (rootfs != NULL) {
            mnt->prev = rootfs->mnt;
        }
        rootfs = dentry_obtain(mnt->dent);
        return 0;
    }

    dent = vfs_lookup(path);
    if (dent == NULL) {
        return -ENOENT;
    }
    copy = dentry_obtain(mnt->dent);
    LIST_ADD(&dent->parent->subdirs, &copy->child);
    copy->parent = dentry_obtain(dent->parent);
    strcpy(copy->name, dent->name);

    if (dent->mnt == NULL) {
        // free original mount point
        dentry_release(dent);
    } else {
        mnt->prev = dent->mnt;
        // delete from tree
        dentry_release(dent->parent);
        dent->parent = NULL;
        LIST_DEL(&dent->child);
        dentry_release(dent);
    }

    return 0;
}

struct mount *vfs_umount(const char *mountpoint)
{
    struct dentry *dent;
    struct mount *mnt, *prev;
    char buf[MAX_PATH_LEN];
    char *path;

    strcpy(buf, mountpoint);
    path = trim(buf);
    if (strcmp(path, PATH_SEP) == 0) {
        if (rootfs == NULL) {
            errno = -ENOENT;
            return NULL;
        }
        dent = rootfs;
        mnt = dent->mnt;
        if (dent->mnt->prev == NULL) {
            rootfs = NULL;
        } else {
            rootfs = dent->mnt->prev->dent;
        }
        dentry_release(dent);
        return mnt;
    }

    dent = vfs_lookup(path);
    if (dent == NULL) {
        errno = -ENOENT;
        return NULL;
    }
    mnt = dent->mnt;
    if (mnt == NULL) {
        errno = -EINVAL;
        return NULL;
    }
    prev = mnt->prev;
    LIST_ADD(&dent->parent->subdirs, &prev->dent->child);
    prev->dent->parent = dentry_obtain(dent->parent);

    dentry_release(dent->parent);
    dent->parent = NULL;
    LIST_DEL(&dent->child);
    dentry_release(dent);

    return mnt;
}

struct dentry *vfs_lookup(const char *pathname)
{
    char buf[MAX_PATH_LEN];
    struct dentry *dent, *from;
    char *token = buf;

    if (pathname == NULL) {
        errno = -EINVAL;
        return NULL;
    }
    if (strlen(pathname) >= MAX_PATH_LEN) {
        errno = -ENAMETOOLONG;
        return NULL;
    }

    strcpy(buf, pathname);
    if (startswith(token, PATH_SEP)) {
        from = dentry_obtain(rootfs);
        (void)strsep(&token, PATH_SEP);
    } else {
        from = dentry_obtain(current->cwd->dent);
    }
    dent = dentry_lookup(from, &token);
    dentry_release(from);
    log_debug("[fs] rootfs->rc=%d\n", rootfs->rc);
    return dent;
}