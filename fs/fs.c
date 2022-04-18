#include <string.h>
#include <errno.h>
#include <libgen.h>

#include <stringex.h>
#include <neuxos.h>
#include <mm/kmalloc.h>

#include "fs.h"

#define dent_accsr(d) ((d)->mnt == NULL ? (d) : ((d)->mnt->dent))

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
        rootfs = dent->mnt->prev;
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
    char *s, *dname;
    char buf[MAX_PATH_LEN];

    if (pathname == NULL) {
        errno = -EINVAL;
        return NULL;
    }
    strcpy(buf, pathname);
    s = buf;
    dname = strsep(&s, PATH_SEP);
    if (strcmp(rootfs->name, dname) != 0) {
        errno = -ENOENT;
        return NULL;
    }
    return dentry_lookup(rootfs, &s);
}

int vfs_mknod(const char *path)
{
    char dname[MAX_PATH_LEN] = {0};
    char fname[MAX_PATH_LEN] = {0};
    struct dentry dent;

    strcpy(dname, path);
    strcpy(fname, path);
    struct dentry *dir = vfs_lookup(dirname(dname));
    if (dir == NULL) {
        return -ENOENT;
    }
    strcpy(dent.name, basename(fname));
    return dent_accsr(dir)->inode->ops->create(dent_accsr(dir)->inode, &dent, 0);
}