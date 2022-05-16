#include <string.h>
#include <errno.h>
#include <libgen.h>

#include <stringex.h>
#include <neuxos.h>
#include <mm/kmalloc.h>
#include <kernel/sched.h>
#include <kernel/log.h>

#include "fs.h"

static struct dentry *rootfs;

void vfs_setup(void)
{
}

int vfs_mount(const char *mountpoint, struct mount *mnt)
{
    struct dentry *dent, *copy;
    char buf[MAX_PATH_LEN];
    char *path;

    strcpy(buf, mountpoint);
    path = trim(buf);
    if (strcmp(path, PATH_SEP) == 0) {
        if (rootfs != NULL) {
            mnt->prev = rootfs->mnt;
        }
        rootfs = dentry_obtain(mnt->dent);
        return 0;
    }

    dent = vfs_lookup(path);
    if (dent == NULL) {
        return -ENOENT;
    }
    copy = dentry_obtain(mnt->dent);
    LIST_ADD(&dent->parent->subdirs, &copy->child);
    copy->parent = dentry_obtain(dent->parent);
    strcpy(copy->name, dent->name);

    if (dent->mnt == NULL) {
        // free original mount point
        dentry_release(dent);
    } else {
        mnt->prev = dent->mnt;
        // delete from tree
        dentry_release(dent->parent);
        dent->parent = NULL;
        LIST_DEL(&dent->child);
        dentry_release(dent);
    }

    return 0;
}

struct mount *vfs_umount(const char *mountpoint)
{
    struct dentry *dent;
    struct mount *mnt, *prev;
    char buf[MAX_PATH_LEN];
    char *path;

    strcpy(buf, mountpoint);
    path = trim(buf);
    if (strcmp(path, PATH_SEP) == 0) {
        if (rootfs == NULL) {
            errno = -ENOENT;
            return NULL;
        }
        dent = rootfs;
        mnt = dent->mnt;
        if (dent->mnt->prev == NULL) {
            rootfs = NULL;
        } else {
            rootfs = dent->mnt->prev->dent;
        }
        dentry_release(dent);
        return mnt;
    }

    dent = vfs_lookup(path);
    if (dent == NULL) {
        errno = -ENOENT;
        return NULL;
    }
    mnt = dent->mnt;
    if (mnt == NULL) {
        errno = -EINVAL;
        return NULL;
    }
    prev = mnt->prev;
    LIST_ADD(&dent->parent->subdirs, &prev->dent->child);
    prev->dent->parent = dentry_obtain(dent->parent);

    dentry_release(dent->parent);
    dent->parent = NULL;
    LIST_DEL(&dent->child);
    dentry_release(dent);

    return mnt;
}

struct dentry *vfs_lookup(const char *pathname)
{
    char buf[MAX_PATH_LEN];
    struct dentry *dent, *from;
    char *token = buf;

    if (pathname == NULL) {
        errno = -EINVAL;
        return NULL;
    }
    if (strlen(pathname) >= MAX_PATH_LEN) {
        errno = -ENAMETOOLONG;
        return NULL;
    }

    strcpy(buf, pathname);
    if (startswith(token, PATH_SEP)) {
        from = dentry_obtain(rootfs);
        (void)strsep(&token, PATH_SEP);
    } else {
        from = dentry_obtain(current->cwd->dent);
    }
    dent = dentry_lookup(from, &token);
    dentry_release(from);
    log_debug("[fs] rootfs->rc=%d\n", rootfs->rc);
    return dent;
}