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
    off_t off;
    struct dentry *dent;
    struct file_ops *ops;
    void *buf;
};

struct file_ops {
    ssize_t (*read)(struct file *pfile, void *buf, size_t count);
    int (*readdir)(struct file *pfile, struct dirent *dent);
    ssize_t (*write)(struct file *pfile, const void *buf, size_t count);
    int (*close)(struct file *pfile);
};

struct file *vfs_open(const char *pathname, int flags);
ssize_t vfs_read(struct file *pfile, void *buf, size_t count);
ssize_t vfs_write(struct file *pfile, const void *buf, size_t count);
int vfs_close(struct file *pfile);
int vfs_readdir(struct file *pfile, struct dirent *dent);

#endif