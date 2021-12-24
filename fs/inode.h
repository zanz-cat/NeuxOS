#ifndef __FS_INODE_H__
#define __FS_INODE_H__

#include <stddef.h>

#include <lib/list.h>

struct dentry;
struct inode_ops;

struct inode {
    struct list_head dentry;
    unsigned long ino;
    size_t size;
    struct inode_ops *ops;
};

struct inode_ops {
    int (*create)(struct inode *dir, struct dentry *dentry, int mode);
    int (*lookup)(struct inode *dir, struct dentry *dentry);
};

#endif