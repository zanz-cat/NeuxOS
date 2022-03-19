#include <errno.h>

#include <mm/kmalloc.h>
#include "dentry.h"
#include "file.h"

static struct mount *mount_search(struct dentry *dentry)
{
    struct dentry *d = dentry;
    while (d != NULL && d->mnt == NULL) {
        d = d->parent;
    }
    return d == NULL ? NULL : d->mnt;
}

struct file *vfs_open(const char *pathname, int flags)
{
    struct dentry *d = dentry_lookup(pathname);
    if (d == NULL) {
        errno = -ENOENT;
        return NULL;
    }
    struct mount *mnt = mount_search(d);
    if (mnt == NULL) {
        dentry_release(d);
        errno = -EPERM;
        return NULL;
    }

    struct file *f = kmalloc(sizeof(struct file));
    if (f == NULL) {
        dentry_release(d);
        errno = -ENOMEM;
        return NULL;
    }
    f->rc = 1;
    f->off = 0;
    f->dentry = d;
    f->ops = &mnt->fs->f_ops;
    f->buf = NULL;
    return f;
}

ssize_t vfs_read(struct file *f, void *buf, size_t count)
{
    ssize_t ret = f->ops->read(f, buf, count);
    if (ret < 0) {
        return ret;
    }
    f->off += ret;
    return ret;
}

ssize_t vfs_write(struct file *f, const void *buf, size_t count)
{
    return 0;
}

int vfs_close(struct file *f)
{
    f->rc--;
    if (f->rc != 0) {
        return 0;
    }
    f->ops->close(f);
    dentry_release(f->dentry);
    kfree(f->buf);
    kfree(f);
    return 0;
}

int vfs_readdir(struct file *f, struct dirent *dent)
{
    int ret;

    if (f->buf == NULL) {
        f->buf = kmalloc(F_INO(f)->size);
        if (f->buf == NULL) {
            return -ENOMEM;
        }
        ret = f->ops->read(f, f->buf, F_INO(f)->size);
        if (ret < 0) {
            return ret;
        }
    }
    if (f->off >= F_INO(f)->size) {
        return -EOF;
    }
    return f->ops->readdir(f, dent);
}