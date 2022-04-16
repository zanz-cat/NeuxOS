#include <errno.h>
#include <string.h>

#include <neuxos.h>
#include <mm/kmalloc.h>
#include <kernel/log.h>

#include "dentry.h"

static struct dentry rootfs = {
    .name = "",
    .inode = NULL,
    .rc = 1,
    .mnt = NULL,
    .lock = SIMPLOCK_INITIALIZER,
    .parent = NULL,
    .alias = { .prev = &rootfs.alias, .next = &rootfs.alias },
    .child = { .prev = &rootfs.child, .next = &rootfs.child },
    .subdirs = { .prev = &rootfs.subdirs, .next = &rootfs.subdirs },
};

static struct dentry *__dentry_lookup(struct dentry *dir, char **token)
{
    struct list_head *child;
    struct dentry *dent;

    if (dir->mnt != NULL) {
        return __dentry_lookup(dir->mnt->dent, token);
    }

    char *dname = strsep(token, PATH_SEP);
    if (dname == NULL || strlen(dname) == 0) {
        return dir;
    }

    simplock_obtain(&dir->lock);
    LIST_FOREACH(&dir->subdirs, child) {
        struct dentry *d = container_of(child, struct dentry, child);
        if (strcmp(d->name, dname) == 0) {
            dent = dentry_obtain(d);
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
    return dent == NULL ? NULL : __dentry_lookup(dent, token);
}

struct dentry *dentry_lookup(const char *pathname)
{
    char *s, *dname;
    char buf[MAX_PATH_LEN];

    if (pathname == NULL) {
        errno = -EINVAL;
        return NULL;
    }
    strcpy(buf, pathname);
    s = buf;
    dname = strsep(&s, PATH_SEP);
    if (strcmp(rootfs.name, dname) != 0) {
        errno = -ENOENT;
        return NULL;
    }
    return __dentry_lookup(&rootfs, &s);
}

void dentry_release(struct dentry *d)
{
    if (d == NULL) {
        return;
    }

    simplock_obtain(&d->lock);
    d->rc--;
    if (d->rc != 0) {
        simplock_release(&d->lock);
        return;
    }

    d->inode->ops->release(d->inode);
    LIST_DEL(&d->child);
    dentry_release(d->parent);  // d->parent == mount, 释放了mount

    simplock_release(&d->lock);
    kfree(d);
}

void dentry_init(struct dentry *d)
{
    d->rc = 1;
    d->inode = NULL;
    d->parent = NULL;
    d->mnt = NULL;
    simplock_init(&d->lock);
    LIST_HEAD_INIT(&d->child);
    LIST_HEAD_INIT(&d->subdirs);
    LIST_HEAD_INIT(&d->alias);
}