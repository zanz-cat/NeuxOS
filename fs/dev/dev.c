#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <kernel/kernel.h>
#include <fs/fs.h>
#include <mm/kmalloc.h>
#include <kernel/log.h>
#include <lib/bitmap.h>

#include "dev.h"

#define DEV_MAX 1024

struct dev_inode {
    struct list_head list;
    char name[DNAME_MAX_LEN];
    struct inode inode;
};

static LIST_HEAD(devlist);
static struct bitmap *bitmap;

static int devfs_i_create(struct inode *dir, struct dentry *dent, int mode);
static int devfs_i_lookup(struct inode *dir, struct dentry *dent);
static ssize_t devfs_f_read(struct file *f, void *buf, size_t count);
static int devfs_f_readdir(struct file *f, struct dirent *dent);
static ssize_t devfs_f_write(struct file *f, const void *buf, size_t count);
static int devfs_f_close(struct file *f);
static struct fs devfs = {
    .name = "devfs",
    .ops = {},
    .i_ops = {
        .create = devfs_i_create,
        .lookup = devfs_i_lookup,
    },
    .f_ops = {
        .read = devfs_f_read,
        .readdir = devfs_f_readdir,
        .write = devfs_f_write,
        .close = devfs_f_close,
    }
};

static int devfs_i_create(struct inode *dir, struct dentry *dent, int mode)
{
    int ino;
    struct dev_inode *dinode;

    ino = bitmap_get(bitmap);
    if (ino < 0) {
        return -ENFILE;
    }

    dinode = kmalloc(sizeof(struct dev_inode));
    if (dinode == NULL) {
        bitmap_put(bitmap, ino);
        return -ENOMEM;
    }

    strcpy(dinode->name, dent->name);
    dinode->inode.priv = dinode;
    dinode->inode.ino = ino;
    dinode->inode.ops = &devfs.i_ops;
    dent->inode = &dinode->inode;
    dir->size += sizeof(struct dev_inode);

    LIST_ADD(&devlist, &dinode->list);

    return 0;
}

static int devfs_i_lookup(struct inode *dir, struct dentry *dent)
{
    struct list_head *node;
    struct dev_inode *dinode;

    LIST_FOREACH(&devlist, node) {
        dinode = container_of(node, struct dev_inode, list);
        if (strcmp(dinode->name, dent->name) == 0) {
            dent->inode = &dinode->inode;
            return 0;
        }
    }
    return -ENOENT;
}

static ssize_t devfs_f_read(struct file *f, void *buf, size_t count)
{
    return 0;
}

static int devfs_f_readdir(struct file *f, struct dirent *dent)
{
    struct dev_inode *dinode;

    if (f->buf == NULL) {
        f->buf = devlist.next;
    }

    if (f->buf == &devlist) {
        return -EOF;
    }

    dinode = container_of(f->buf, struct dev_inode, list);
    dent->d_ino = dinode->inode.ino;
    dent->d_off = 0;
    dent->d_type = DT_CHR;
    dent->d_reclen = sizeof(struct dev_inode);
    strcpy(dent->d_name, dinode->name);
    f->off += sizeof(struct dev_inode);
    f->buf = dinode->list.next;

    return 0;
}

static ssize_t devfs_f_write(struct file *f, const void *buf, size_t count)
{
    return 0;
}

static int devfs_f_close(struct file *f)
{
    return 0;
}

static void devfs_mount(void)
{
    int ret;
    struct mount *mnt = NULL;
    const char *err = NULL;

    mnt = kmalloc(sizeof(struct mount));
    if (mnt == NULL) {
        err = "no memory for mount";
        goto panic;
    }
    mnt->fs = &devfs;
    mnt->dentry = kmalloc(sizeof(struct dentry));
    if (mnt->dentry == NULL) {
        err = "no memory for devfs dentry";
        goto panic;
    }
    dentry_init(mnt->dentry);
    mnt->dentry->rc = 1;
    mnt->dentry->inode = kmalloc(sizeof(struct inode));
    if (mnt->dentry->inode == NULL) {
        err = "no memory for devfs inode";
        goto panic;
    }
    mnt->dentry->inode->ino = 0;
    mnt->dentry->inode->size = 0;
    mnt->dentry->inode->ops = &devfs.i_ops;
    LIST_HEAD_INIT(&mnt->dentry->inode->dentry);
    LIST_ADD(&mnt->dentry->inode->dentry, &mnt->dentry->alias);

    ret = vfs_mount("/dev", mnt);
    if (ret != 0) {
        err = "mount devfs error";
        goto panic;
    }
    log_info("dev fs mounted.\n");
    return;

panic:
    if (mnt != NULL) {
        if (mnt->dentry != NULL) {
            kfree(mnt->dentry);
        }
        kfree(mnt);
    }
    kernel_panic("err=%s, code=%d\n", err, ret);
}

void devfs_setup(void)
{
    size_t msize = bitmap_msize(DEV_MAX);
    bitmap = kmalloc(msize);
    if (bitmap == NULL) {
        kernel_panic("err=alloc dev bitmap\n");
    }
    bitmap_init(bitmap, DEV_MAX);
    bitmap_get(bitmap); // 0 is reserved for root
    devfs_mount();
}

void devfs_teardown(void)
{
    // devfs_umount; 
    kfree(bitmap);
}