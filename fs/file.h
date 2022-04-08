#ifndef __FS_FILE_H__
#define __FS_FILE_H__

#include <stdint.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

struct file_ops;

struct file {
    volatile uint16_t rc;
    mode_t f_mode;
    off_t off;
    struct dentry *dentry;
    struct file_ops *ops;
    void *buf;
};

#define F_INO(f) ((f)->dentry->inode)

struct file_ops {
    ssize_t (*read)(struct file *f, void *buf, size_t count);
    int (*readdir)(struct file *f, struct dirent *dent);
    ssize_t (*write)(struct file *f, const void *buf, size_t count);
    int (*close)(struct file *f);
};

struct file *vfs_open(const char *pathname, int flags);
ssize_t vfs_read(struct file *f, void *buf, size_t count);
ssize_t vfs_write(struct file *f, const void *buf, size_t count);
int vfs_close(struct file *f);
int vfs_readdir(struct file *f, struct dirent *dent);

#endif