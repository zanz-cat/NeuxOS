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
    struct file *pfile;
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

    pfile = kmalloc(sizeof(struct file));
    if (pfile == NULL) {
        errno = -ENOMEM;
        goto error;
    }
    pfile->rc = 1;
    pfile->off = 0;
    pfile->dent = dent;
    pfile->ops = &mnt->fs->f_ops;
    pfile->buf = NULL;
    return pfile;

error:
    dentry_release(dent);
    return NULL;
}

int vfs_close(struct file *pfile)
{
    pfile->rc--;
    if (pfile->rc != 0) {
        return 0;
    }
    dentry_release(pfile->dent);
    op_assert(pfile->ops->close);
    pfile->ops->close(pfile);
    kfree(pfile);
    return 0;
}

ssize_t vfs_read(struct file *pfile, void *buf, size_t count)
{
    op_assert(pfile->ops->read);

    ssize_t ret = pfile->ops->read(pfile, buf, count);
    if (ret < 0) {
        return ret;
    }
    pfile->off += ret;
    return ret;
}

ssize_t vfs_write(struct file *pfile, const void *buf, size_t count)
{
    op_assert(pfile->ops->write);
    return pfile->ops->write(pfile, buf, count);
}

int vfs_readdir(struct file *pfile, struct dirent *dent)
{
    op_assert(pfile->ops->readdir);
    return pfile->ops->readdir(pfile, dent);
}