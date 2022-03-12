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
    f->offset = 0;
    f->dentry = d;
    f->ops = &mnt->fs->f_ops;
    return f;
}

int vfs_read(struct file *f, void *buf, size_t count)
{
    return f->ops->read(f, buf, count);
}

int vfs_write(struct file *f, const void *buf, size_t count)
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
    kfree(f);
    return 0;
}