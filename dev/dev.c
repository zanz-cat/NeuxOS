#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

#include <neuxos.h>
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
    uint16_t dno;
    struct inode inode;
};

static LIST_HEAD(dev_list);

static LIST_HEAD(inode_list);
static struct bitmap *inode_bitmap;

static int devfs_i_create(struct inode *dir, struct dentry *dent, int mode);
static int devfs_i_lookup(struct inode *dir, struct dentry *dent);
static void devfs_i_release(struct inode *inode);

static ssize_t devfs_f_read(struct file *pfile, void *buf, size_t count);
static int devfs_f_readdir(struct file *pfile, struct dirent *dent);
static ssize_t devfs_f_write(struct file *pfile, const void *buf, size_t count);
static int devfs_f_close(struct file *pfile);
static struct fs devfs = {
    .name = "devfs",
    .ops = {},
    .i_ops = {
        .create = devfs_i_create,
        .lookup = devfs_i_lookup,
        .release = devfs_i_release,
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

    ino = bitmap_get(inode_bitmap);
    if (ino < 0) {
        return -ENFILE;
    }

    dinode = kmalloc(sizeof(struct dev_inode));
    if (dinode == NULL) {
        bitmap_put(inode_bitmap, ino);
        return -ENOMEM;
    }

    strcpy(dinode->name, dent->name);
    dinode->inode.priv = dinode;
    dinode->inode.ino = ino;
    dinode->inode.ops = &devfs.i_ops;
    dent->inode = &dinode->inode;
    dir->size += sizeof(struct dev_inode);

    LIST_ADD(&inode_list, &dinode->list);

    return 0;
}

static int devfs_i_lookup(struct inode *dir, struct dentry *dent)
{
    struct list_head *node;
    struct dev_inode *dinode;

    LIST_FOREACH(&inode_list, node) {
        dinode = container_of(node, struct dev_inode, list);
        if (strcmp(dinode->name, dent->name) == 0) {
            dent->inode = &dinode->inode;
            return 0;
        }
    }
    return -ENOENT;
}

static void devfs_i_release(struct inode *inode)
{
    // nop
}

static struct dev *dev_lookup(uint16_t dno)
{
    struct list_head *node;
    struct dev *devp;

    LIST_FOREACH(&dev_list, node) {
        devp = container_of(node, struct dev, list);
        if (devp->dno == dno) {
            return devp;
        }
    }
    return NULL;
}

static ssize_t devfs_f_read(struct file *pfile, void *buf, size_t count)
{
    struct dev_inode *dinode;
    struct dev *devp;

    dinode = pfile->dent->inode->priv;
    devp = dev_lookup(dinode->dno);
    if (devp == NULL) {
        return -ENODEV;
    }
    if (devp->type == DEV_T_CDEV) {
        return devp->ops.cdev->read(devp, buf, count);
    }
    return -ENOTSUP;
}

static int devfs_f_readdir(struct file *pfile, struct dirent *dent)
{
    struct dev_inode *dinode;

    if (pfile->buf == NULL) {
        pfile->buf = inode_list.next;
    }

    if (pfile->buf == &inode_list) {
        return -EOF;
    }

    dinode = container_of(pfile->buf, struct dev_inode, list);
    dent->d_ino = dinode->inode.ino;
    dent->d_off = 0;
    dent->d_type = DT_CHR;
    dent->d_reclen = sizeof(struct dev_inode);
    strcpy(dent->d_name, dinode->name);
    pfile->off += sizeof(struct dev_inode);
    pfile->buf = dinode->list.next;

    return 0;
}

static ssize_t devfs_f_write(struct file *pfile, const void *buf, size_t count)
{
    struct dev_inode *dinode;
    struct dev *devp;

    dinode = pfile->dent->inode->priv;
    devp = dev_lookup(dinode->dno);
    if (devp == NULL) {
        return -ENODEV;
    }
    if (devp->type == DEV_T_CDEV) {
        return devp->ops.cdev->write(devp, buf, count);
    }
    return -ENOTSUP;
}

static int devfs_f_close(struct file *pfile)
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
    mnt->prev = NULL;
    mnt->dent = kmalloc(sizeof(struct dentry));
    if (mnt->dent == NULL) {
        err = "no memory for devfs dentry";
        goto panic;
    }
    dentry_init(mnt->dent);
    mnt->dent->rc = 1;
    mnt->dent->mnt = mnt;
    mnt->dent->inode = kmalloc(sizeof(struct inode));
    if (mnt->dent->inode == NULL) {
        err = "no memory for devfs inode";
        goto panic;
    }
    inode_init(mnt->dent->inode, 0, S_IFDIR, 0, &devfs.i_ops);
    LIST_ADD(&mnt->dent->inode->dent, &mnt->dent->alias);

    ret = vfs_mount("/dev", mnt);
    if (ret != 0) {
        err = "mount devfs error";
        goto panic;
    }
    log_info("devfs mounted.\n");
    return;

panic:
    if (mnt != NULL) {
        if (mnt->dent != NULL) {
            kfree(mnt->dent);
        }
        kfree(mnt);
    }
    kernel_panic("err=%s, code=%d\n", err, ret);
}

void dev_init(struct dev *devp)
{
    devp->rc = 1;
    LIST_HEAD_INIT(&devp->list);
    devp->ops.cdev = NULL;
    devp->priv = NULL;
}

int dev_add(struct dev *devp)
{
    if (dev_lookup(devp->dno) != NULL) {
        return -EEXIST;
    }
    LIST_ADD(&dev_list, &devp->list);
    return 0;
}

struct dev *dev_del(uint16_t dno)
{
    struct dev *devp;
    
    devp = dev_lookup(dno);
    if (devp == NULL) {
        errno = -ENOENT;
        return NULL;
    }
    LIST_DEL(&devp->list);
    return devp;
}

int devfs_mknod(const char *path, uint16_t dno)
{
    int ret;
    char dname[MAX_PATH_LEN] = {0};
    char fname[MAX_PATH_LEN] = {0};
    struct dentry dent;

    strcpy(dname, path);
    strcpy(fname, path);
    struct dentry *dir = vfs_lookup(dirname(dname));
    if (dir == NULL) {
        return -ENOENT;
    }
    strcpy(dent.name, basename(fname));
    ret = dir->inode->ops->create(dir->inode, &dent, 0);
    if (ret != 0) {
        return ret;
    }
    ((struct dev_inode *)dent.inode->priv)->dno = dno;
    return 0;
}

void devfs_setup(void)
{
    size_t msize = bitmap_msize(DEV_MAX);

    inode_bitmap = kmalloc(msize);
    if (inode_bitmap == NULL) {
        kernel_panic("err=alloc inode bitmap\n");
    }
    bitmap_init(inode_bitmap, DEV_MAX);
    bitmap_get(inode_bitmap); // 0 is reserved for root

    devfs_mount();
}

void devfs_teardown(void)
{
    // devfs_umount; 
    kfree(inode_bitmap);
}