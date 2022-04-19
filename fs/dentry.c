#include <errno.h>
#include <string.h>

#include <neuxos.h>
#include <mm/kmalloc.h>
#include <kernel/log.h>

#include "dentry.h"

struct dentry *dentry_lookup(struct dentry *dir, char **token)
{
    struct list_head *child;
    struct dentry *dent, *subdent;

    char *dname = strsep(token, PATH_SEP);
    if (dname == NULL || strlen(dname) == 0) {
        return dentry_obtain(dir);
    }

    if (strcmp(dname, ".") == 0) {
        return dentry_obtain(dir);
    }
    if (strcmp(dname, "..") == 0) {
        return dir->parent == NULL ? dentry_obtain(dir) : dentry_lookup(dir->parent, token);
    }

    simplock_obtain(&dir->lock);
    LIST_FOREACH(&dir->subdirs, child) {
        subdent = container_of(child, struct dentry, child);
        if (strcmp(subdent->name, dname) == 0) {
            dent = dentry_obtain(subdent);
            goto out;
        }
    }
    // found it
    dent = kmalloc(sizeof(struct dentry));
    if (dent == NULL) {
        errno = -ENOMEM;
        goto out;
    }
    dentry_init(dent);
    strncpy(dent->name, dname, DNAME_MAX_LEN);
    int ret = dir->inode->ops->lookup(dir->inode, dent);
    if (ret != 0) {
        errno = -ENOENT;
        kfree(dent); // no reference to anything, safe to free
        dent = NULL;
        goto out;
    }
    dent->mnt = dir->mnt;
    dent->parent = dentry_obtain(dir);
    LIST_ADD(&dir->subdirs, &dentry_obtain(dent)->child);

out:
    simplock_release(&dir->lock);
    return dent == NULL ? NULL : dentry_lookup(dent, token);
}

struct dentry *dentry_obtain(struct dentry *dent)
{
    dent->rc++;
    return dent;
}

void dentry_release(struct dentry *dent)
{
    if (dent == NULL) {
        return;
    }

    simplock_obtain(&dent->lock);
    dent->rc--;
    if (dent->rc != 0) {
        simplock_release(&dent->lock);
        return;
    }

    dent->inode->ops->release(dent->inode);
    if (dent->parent != NULL) {
        LIST_DEL(&dent->child);
        dentry_release(dent->parent);
        dent->parent = NULL;
    }

    simplock_release(&dent->lock);
    kfree(dent);
}

static const char *_search(const struct dentry *dent, char **pbuf, char *end)
{
    const char *p;

    if (dent == NULL) {
        return NULL;
    }

    p = _search(dent->parent, pbuf, end);
    if (errno != 0) {
        return NULL;
    }
    if (p == NULL) {
        return dent->name;
    }

    if (*pbuf + strlen(p) + 1 >= end) {
        errno = -ENOMEM;
        return NULL;
    }
    *pbuf += sprintf(*pbuf, "%s" PATH_SEP, p);
    return dent->name;
}

int dentry_abspath(const struct dentry *dent, char *buf, size_t size)
{
    const char *p;
    char *pbuf = buf;

    errno = 0;
    p = _search(dent, &pbuf, buf + size);
    if (errno != 0) {
        return errno;
    }
    if (pbuf + strlen(p) + 1 >= buf + size) {
        return -ENOMEM;
    }
    strcpy(pbuf, p);
    return 0;
}

void dentry_init(struct dentry *dent)
{
    dent->rc = 1;
    dent->inode = NULL;
    dent->parent = NULL;
    dent->mnt = NULL;
    simplock_init(&dent->lock);
    LIST_HEAD_INIT(&dent->child);
    LIST_HEAD_INIT(&dent->subdirs);
    LIST_HEAD_INIT(&dent->alias);
}