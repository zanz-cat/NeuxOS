#include <errno.h>

#include <mm/kmalloc.h>
#include "dentry.h"
#include "file.h"

#define op_assert(fn) \
__extension__({ \
    if ((fn) == NULL) { \
        return -ENOTSUP; \
    } \
})

static struct mount *dentry_mnt(struct dentry *dent)
{
    struct dentry *p = dent;
    while (p != NULL && p->mnt == NULL) {
        p = p->parent;
    }
    return p == NULL ? NULL : p->mnt;
}

struct file *vfs_open(const char *pathname, int flags)
{
    struct mount *mnt;
    struct dentry *dent = NULL;
    
    dent = vfs_lookup(pathname);
    if (dent == NULL) {
        errno = -ENOENT;
        goto error;
    }
    mnt = dentry_mnt(dent);
    if (mnt == NULL) {
        errno = -EPERM;
        goto error;
    }

    struct file *f = kmalloc(sizeof(struct file));
    if (f == NULL) {
        errno = -ENOMEM;
        goto error;
    }
    f->rc = 1;
    f->off = 0;
    f->dent = dent;
    f->ops = &mnt->fs->f_ops;
    f->buf = NULL;
    return f;

error:
    dentry_release(dent);
    return NULL;
}

int vfs_close(struct file *f)
{
    f->rc--;
    if (f->rc != 0) {
        return 0;
    }
    dentry_release(f->dent);
    op_assert(f->ops->close);
    f->ops->close(f);
    kfree(f);
    return 0;
}

ssize_t vfs_read(struct file *f, void *buf, size_t count)
{
    op_assert(f->ops->read);

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

int vfs_readdir(struct file *f, struct dirent *dent)
{
    op_assert(f->ops->readdir);
    return f->ops->readdir(f, dent);
}