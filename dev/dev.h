#ifndef __FS_DEV__
#define __FS_DEV__
#include <stddef.h>

#include <lib/list.h>

#define DEV_T_CDEV 0
#define DEV_T_BLK 1

struct dev;

struct cdev_ops {
    int (*open)(struct dev *devp);
    int (*close)(struct dev *devp);
    ssize_t (*write)(struct dev *devp, const char *buf, size_t count);
    ssize_t (*read)(struct dev *devp, char *buf, size_t count);
};

struct bdev_ops {

};

struct dev {
    struct list_head list;
    volatile uint32_t rc;
    uint16_t dno;
    uint8_t type;
    union {
        struct cdev_ops *cdev;
        struct bdev_ops *bdev;
    } ops;
    void *priv;
};

void devfs_setup(void);
int devfs_mknod(const char *path, uint16_t dno);

void dev_init(struct dev *devp);
int dev_add(struct dev *devp);
struct dev *dev_del(uint16_t dno);

#endif