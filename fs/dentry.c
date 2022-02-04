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

static inline struct dentry *dentry_obtain(struct dentry *d)
{
    d->rc++;
    return d;
}

static struct dentry *__dentry_lookup(struct dentry *dir, char **token)
{
    if (dir->mnt != NULL) {
        return __dentry_lookup(dir->mnt->dentry, token);
    }

    char *dname = strsep(token, PATH_SEP);
    if (dname == NULL || strlen(dname) == 0) {
        return dir;
    }

    struct dentry *dentry = kmalloc(sizeof(struct dentry));
    if (dentry == NULL) {
        errno = -ENOMEM;
        return NULL;
    }
    dentry_init(dentry);
    strncpy(dentry->name, dname, DNAME_MAX_LEN);
    dentry->rc = 1;
    dentry->inode = NULL;
    dentry->mnt = NULL;
    dentry->parent = dentry_obtain(dir);

    int ret = dir->inode->ops->lookup(dir->inode, dentry);
    if (ret != 0) {
        dentry_free(dentry);
        errno = -ENOENT;
        return NULL;
    }
    return __dentry_lookup(dentry, token);
}

struct dentry *dentry_lookup(const char *pathname)
{
    char *s, *dname;
    char buf[MAX_PATH_LEN];

    if (pathname == NULL) {
        errno = -EINVAL;
        return NULL;
    }
    strncpy(buf, pathname, MAX_PATH_LEN);
    s = buf;
    dname = strsep(&s, PATH_SEP);
    if (strcmp(rootfs.name, dname) != 0) {
        errno = -ENOENT;
        return NULL;
    }
    return __dentry_lookup(&rootfs, &s);
}

void dentry_free(struct dentry *d)
{
    d->rc--;
    if (d->rc != 0) {
        return;
    }
    if (d->parent != NULL) {
        dentry_free(d->parent);
    }
    kfree(d);
}

void dentry_init(struct dentry *d)
{
    d->rc = 0;
    d->inode = NULL;
    d->parent = NULL;
    d->mnt = NULL;
    LIST_HEAD_INIT(&d->child);
    LIST_HEAD_INIT(&d->subdirs);
    LIST_HEAD_INIT(&d->alias);
}