#ifndef __FS_INODE_H__
#define __FS_INODE_H__

#include <stddef.h>
#include <stdint.h>

#include <lib/list.h>

struct dentry;
struct inode_ops;

struct inode {
    struct list_head dent;  // linked to dentry.alias
    unsigned long ino;
    uint16_t mode;
    size_t size;
    struct inode_ops *ops;
    void *priv;
};

struct inode_ops {
    int (*create)(struct inode *dir, struct dentry *dent, int mode);
    int (*lookup)(struct inode *dir, struct dentry *dent);
    void (*release)(struct inode *inode);
};

static inline inode_init(struct inode *inode,
            unsigned long ino, uint16_t mode,
            size_t size, struct inode_ops *ops)
{
    LIST_HEAD_INIT(&inode->dent);
    inode->ino = ino;
    inode->mode = mode;
    inode->size = size;
    inode->ops = ops;
    inode->priv = NULL;
}

#endif