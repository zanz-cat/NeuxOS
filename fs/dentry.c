#include <errno.h>
#include <string.h>

#include <neuxos.h>
#include <mm/kmalloc.h>
#include <kernel/log.h>

#include "dentry.h"

static struct dentry rootfs = {
    .name = "",
    .mnt = NULL,
    .parent = NULL,
    .inode = NULL,
    .alias = { .prev = &rootfs.alias, .next = &rootfs.alias },
    .child = { .prev = &rootfs.child, .next = &rootfs.child },
    .subdirs = { .prev = &rootfs.subdirs, .next = &rootfs.subdirs },
};

static struct dentry *__dentry_lookup(struct dentry *dir, char **token)
{
    if (dir->mnt != NULL) {
        return __dentry_lookup(dir->mnt, token);
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
    memset(dentry, 0, sizeof(struct dentry));
    dentry->name = dname;

    int ret = dir->inode->ops->lookup(dir->inode, dentry);
    if (ret != 0) {
        kfree(dentry);
        errno = -ENOENT;
        return NULL;
    }
    struct dentry *res = __dentry_lookup(dentry, token);
    if (dentry != res) {
        kfree(dentry);
    }
    return res;
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
    if (d->rc == 0) {
        kfree(d);
    }
}

void dentry_init(struct dentry *d)
{
    d->child.prev = &d->child;
    d->child.next = &d->child;
    d->subdirs.prev = &d->subdirs;
    d->subdirs.next = &d->subdirs;
    d->alias.prev = &d->alias;
    d->alias.next = &d->alias;
}