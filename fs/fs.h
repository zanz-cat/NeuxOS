#ifndef __FS_FS_H__
#define __FS_FS_H__

#include <stdint.h>
#include <stdio.h>
#include <lib/list.h>

#include "file.h"
#include "dentry.h"
#include "inode.h"

#define FS_NAME_LEN 16

struct fs_ops;

struct fs {
    char name[FS_NAME_LEN];
    struct fs_ops *ops;
};

struct fs_ops {

};

void vfs_setup(void);
int vfs_mount(const char *mountpoint, struct dentry *mnt_root);
int vfs_umount(const char *mountpoint);

#endif