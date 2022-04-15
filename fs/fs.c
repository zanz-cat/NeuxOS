#include <string.h>
#include <errno.h>
#include <libgen.h>

#include <neuxos.h>
#include <mm/kmalloc.h>

#include "fs.h"

#define dent_accsr(d) ((d)->mnt == NULL ? (d) : ((d)->mnt->dent))

void vfs_setup(void)
{
}

int vfs_mount(const char *mountpoint, struct mount *mount)
{
    struct dentry *mp = dentry_lookup(mountpoint);
    if (mp == NULL) {
        return -ENOENT;
    }

    if (mp->mnt != NULL) {
        LIST_DEL(&mp->mnt->dent->child); // delete from parent's subdirs
        mount->override = mp->mnt;
    }

    mp->mnt = mount;
    mount->dent->parent = dentry_obtain(mp);
    if (mp->parent != NULL) {
        LIST_ADD(&mp->parent->subdirs, &dentry_obtain(mount->dent)->child);
    }
    strcpy(mount->dent->name, mp->name);

    return 0;
}

struct mount *vfs_umount(const char *mountpoint)
{
    struct dentry *mp = dentry_lookup(mountpoint);
    if (mp == NULL) {
        errno = -ENOENT;
        return NULL;
    }
    struct mount *mnt = mp->mnt;
    mp->mnt = mnt->override;
    dentry_release(mnt->dent);
    dentry_release(mnt->dent->parent);
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