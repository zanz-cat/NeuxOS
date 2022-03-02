#ifndef __FS_FILE_H__
#define __FS_FILE_H__

#include <stdint.h>
#include <stdio.h>
#include <errno.h>

struct file_ops;

struct file {
    volatile uint16_t rc;
    size_t offset;
    struct dentry *dentry;
    struct file_ops *ops;
};

struct file_ops {
    ssize_t (*read)(struct file *f, void *buf, size_t count);
    ssize_t (*write)(struct file *f, const void *buf, size_t count);
    int (*close)(struct file *f);
};

struct file *vfs_open(const char *pathname, int flags);
int vfs_read(struct file *f, void *buf, size_t count);
int vfs_write(struct file *f, const void *buf, size_t count);
int vfs_close(struct file *f);

#endif