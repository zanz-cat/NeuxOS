#include <string.h>
#include <errno.h>

#include <neuxos.h>
#include "fs.h"

void vfs_setup(void)
{
}

int vfs_mount(const char *mountpoint, struct dentry *mnt_root)
{
    struct dentry *d = dentry_lookup(mountpoint);
    if (d == NULL) {
        return -ENOENT;
    }
    d->mnt = mnt_root;
    return 0;
}

int vfs_umount(const char *mountpoint)
{
    struct dentry *target = dentry_lookup(mountpoint);
    if (target == NULL) {
        return -ENOENT;
    }
    target->mnt = NULL;
    return 0;
}