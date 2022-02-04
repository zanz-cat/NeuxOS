#ifndef __FS_FS_H__
#define __FS_FS_H__

#include <stdint.h>
#include <stdio.h>
#include <lib/list.h>

#include "dentry.h"
#include "file.h"

#define FS_NAME_LEN 16

struct fs_ops {

};

struct fs {
    char name[FS_NAME_LEN];
    struct fs_ops ops;
    struct file_ops f_ops;
};

struct mount {
    struct fs *fs;
    struct dentry *dentry;
};

void vfs_setup(void);
int vfs_mount(const char *mountpoint, struct mount *mount);
struct mount *vfs_umount(const char *mountpoint);

#endif