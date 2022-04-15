#ifndef __FS_DENTRY_H__
#define __FS_DENTRY_H__

#include <stdint.h>

#include <lib/list.h>

#include <kernel/lock.h>
#include "inode.h"
#include "fs/fs.h"

#define DNAME_MAX_LEN 16

struct dentry {
    char name[DNAME_MAX_LEN];
    volatile uint16_t rc;
    struct inode *inode;
    struct dentry *parent;
    struct mount *mnt;
    struct simplock lock;
    struct list_head child;
    struct list_head subdirs;
    struct list_head alias;
};

struct dentry_ops {

};

static inline struct dentry *dentry_obtain(struct dentry *d)
{
    d->rc++;
    return d;
}

struct dentry *dentry_lookup(const char *pathname);
void dentry_release(struct dentry *d);
void dentry_init(struct dentry *d);

#endif