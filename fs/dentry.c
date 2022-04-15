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

    LIST_FOREACH(&dir->subdirs, child) {
        struct dentry *d = container_of(child, struct dentry, child);
        if (strcmp(d->name, dname) == 0) {
            dent = dentry_obtain(d);
            goto lookup_subdir;
        }
    }

    dent = kmalloc(sizeof(struct dentry));
    if (dent == NULL) {
        errno = -ENOMEM;
        return NULL;
    }
    dentry_init(dent);
    strncpy(dent->name, dname, DNAME_MAX_LEN);

    int ret = dir->inode->ops->lookup(dir->inode, dent);
    if (ret != 0) {
        dentry_release(dent);
        errno = -ENOENT;
        return NULL;
    }

    dent->mnt = dir->mnt;
    dent->parent = dentry_obtain(dir);
    LIST_ADD(&dir->subdirs, &dentry_obtain(dent)->child);

lookup_subdir:
    return __dentry_lookup(dent, token);
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

    d->rc--;
    if (d->rc != 0) {
        return;
    }
    d->inode->ops->release(d->inode);
    dentry_release(d->parent);
    kfree(d);
}

void dentry_init(struct dentry *d)
{
    d->rc = 1;
    d->inode = NULL;
    d->parent = NULL;
    d->mnt = NULL;
    LIST_HEAD_INIT(&d->child);
    LIST_HEAD_INIT(&d->subdirs);
    LIST_HEAD_INIT(&d->alias);
}