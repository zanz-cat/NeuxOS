#ifndef __FS_DENTRY_H__
#define __FS_DENTRY_H__

#include <stdint.h>

#include <lib/list.h>

#include "inode.h"

struct dentry {
    char *name;
    struct list_head child;
    struct list_head subdirs;
    struct list_head alias;
    struct inode *inode;
    volatile uint16_t rc;
    struct dentry *mnt;
    struct dentry *parent;
};

struct dentry_ops {

};

struct dentry *dentry_lookup(const char *pathname);
void dentry_free(struct dentry *d);
void dentry_init(struct dentry *d);

#endif