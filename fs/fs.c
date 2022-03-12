#include <string.h>
#include <errno.h>
#include <libgen.h>

#include <neuxos.h>
#include <mm/kmalloc.h>

#include "fs.h"

void vfs_setup(void)
{
}

int vfs_mount(const char *mountpoint, struct mount *mount)
{
    struct dentry *d = dentry_lookup(mountpoint);
    if (d == NULL) {
        return -ENOENT;
    }
    mount->dentry->parent = d;
    d->mnt = mount;
    return 0;
}

struct mount *vfs_umount(const char *mountpoint)
{
    struct dentry *target = dentry_lookup(mountpoint);
    if (target == NULL) {
        errno = -ENOENT;
        return NULL;
    }
    struct mount *res = target->mnt;
    target->mnt = NULL;
    return res;
}

int vfs_mknod(const char *path)
{
    char copy[MAX_PATH_LEN] = {0};
    struct dentry dent;

    strcpy(copy, path);
    struct dentry *dir = dentry_lookup(dirname(copy));
    if (dir == NULL) {
        return -ENOENT;
    }
    strcpy(dent.name, basename(copy));
    return dir->inode->ops->create(dir->inode, &dent, 0);
}