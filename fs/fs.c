#include <string.h>
#include <errno.h>
#include <libgen.h>

#include <neuxos.h>
#include <mm/kmalloc.h>

#include "fs.h"

#define dent_accsr(d) ((d)->mnt == NULL ? (d) : ((d)->mnt->dentry))

void vfs_setup(void)
{
}

int vfs_mount(const char *mountpoint, struct mount *mount)
{
    struct dentry *dent = dentry_lookup(mountpoint);
    if (dent == NULL) {
        return -ENOENT;
    }

    if (dent->mnt != NULL) {
        LIST_DEL(&dent->mnt->dentry->child); // delete from parent's subdirs
        dentry_release(dent->mnt->dentry);
    }

    dent->mnt = mount;
    mount->dentry->parent = dentry_obtain(dent);
    if (dent->parent != NULL) {
        LIST_ADD(&dent->parent->subdirs, &dentry_obtain(mount->dentry)->child);
    }

    return 0;
}

struct mount *vfs_umount(const char *mountpoint)
{
    struct dentry *target = dentry_lookup(mountpoint);
    if (target == NULL) {
        errno = -ENOENT;
        return NULL;
    }
    struct mount *mnt = target->mnt;
    target->mnt = NULL;
    dentry_release(mnt->dentry);
    dentry_release(mnt->dentry->parent);
    return mnt;
}

int vfs_mknod(const char *path)
{
    char dname[MAX_PATH_LEN] = {0};
    char fname[MAX_PATH_LEN] = {0};
    struct dentry dent;

    strcpy(dname, path);
    strcpy(fname, path);
    struct dentry *dir = dentry_lookup(dirname(dname));
    if (dir == NULL) {
        return -ENOENT;
    }
    strcpy(dent.name, basename(fname));
    return dent_accsr(dir)->inode->ops->create(dent_accsr(dir)->inode, &dent, 0);
}