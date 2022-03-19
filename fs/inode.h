#ifndef __FS_INODE_H__
#define __FS_INODE_H__

#include <stddef.h>
#include <stdint.h>

#include <lib/list.h>

struct dentry;
struct inode_ops;

struct inode {
    struct list_head dentry;
    unsigned long ino;
    uint16_t mode;
    size_t size;
    struct inode_ops *ops;
    void *priv;
};

struct inode_ops {
    int (*create)(struct inode *dir, struct dentry *dentry, int mode);
    int (*lookup)(struct inode *dir, struct dentry *dentry);
};

#endif