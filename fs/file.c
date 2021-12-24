#include <errno.h>

#include <mm/kmalloc.h>
#include "dentry.h"
#include "file.h"

struct file *vfs_open(const char *pathname, int flags)
{
    struct dentry *d = dentry_lookup(pathname);
    if (d == NULL) {
        errno = -ENOENT;
        return NULL;
    }

    struct file *f = kmalloc(sizeof(struct file));
    if (f == NULL) {
        errno = -ENOMEM;
        return NULL;
    }
    f->rc = 0;
    f->offset = 0;
    f->dentry = d;
    f->ops = NULL;

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
    if (f->rc == 0) {
        dentry_free(f->dentry);
        kfree(f);
    }
    return 0;
}