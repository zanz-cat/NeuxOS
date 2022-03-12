#include <kernel/kernel.h>
#include <fs/fs.h>
#include <mm/kmalloc.h>
#include <kernel/log.h>

#include "dev.h"

static ssize_t devfs_read(struct file *f, void *buf, size_t count)
{
return 0;
}

static ssize_t devfs_write(struct file *f, const void *buf, size_t count)
{
    return 0;
}

static int devfs_close(struct file *f)
{
    return 0;
}

static void devfs_mount(void)
{
    static struct fs devfs = {
        .name = "devfs",
        .ops = {},
        .f_ops = {
            .read = devfs_read,
            .write = devfs_write,
            .close = devfs_close,
        }
    };

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
    mnt->dentry->inode->ops = NULL;
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
    devfs_mount();
}