#include <string.h>
#include <errno.h>

#include <neuxos.h>
#include <kernel/log.h>
#include <kernel/printk.h>
#include <kernel/interrupt.h>
#include <kernel/kernel.h>
#include <mm/kmalloc.h>
#include <drivers/harddisk.h>
#include <lib/list.h>
#include <misc/misc.h>
#include <fs/fs.h>

#include "ext2.h"

#define CACHE_SIZE 1000

static uint32_t part_lba;
static struct ext2_super_block *s_block;
static struct ext2_block_group *b_groups;
static uint32_t groups_count;
static uint32_t inodes_per_block = CONFIG_EXT2_BS/sizeof(struct ext2_inode);
static LIST_HEAD(inode_cache);
static struct inode_ops ext2_inode_ops;

static inline int read_block(uint32_t block, uint32_t count, void *buf, size_t size)
{
    return hd_read(part_lba+(CONFIG_EXT2_BS/CONFIG_HD_SECT_SZ)*block, 
        count*CONFIG_EXT2_BS/CONFIG_HD_SECT_SZ, buf, size);
}

static int read_inode(uint32_t ino, struct ext2_inode *inode)
{
    int ret;
    struct ext2_inode inode_table[inodes_per_block];
    static struct ext2_block_group *group;

    group = b_groups + (ino-s_block->first_data_block)/s_block->inodes_per_group;
    ino = (ino-s_block->first_data_block)%s_block->inodes_per_group;
    ret = read_block(group->inode_table+ino/inodes_per_block, 1, 
                     inode_table, sizeof(inode_table));
    if (ret != 0) {
        log_error("Ext2: read block error, errno: %d\n", ret);
        return ret;
    }
    *inode = inode_table[ino%inodes_per_block];
    return 0;
}

static int read_s_block(void)
{
    int i, ret;
    struct hd_mbr mbr;
    struct hd_mbr_part *partition = NULL;

    ret = hd_read(0, 1, &mbr, sizeof(struct hd_mbr));
    if (ret != 0) {
        return ret;
    }
    if (*(uint16_t*)mbr.end_magic != HD_MBR_END_MAGIC) {
        return -EINVAL;
    }
    for (i = 0; i < HD_MBR_DPT_SIZE; i++) {
        if (mbr.DPT[i].boot == PART_BOOTABLE) {
            partition = &mbr.DPT[i];
        }
    }
    if (partition == NULL) {
        return -ENONET;
    }
    part_lba = partition->lba;

    s_block = kmalloc(sizeof(struct ext2_super_block));
    if (s_block == NULL) {
        return -ENOMEM;
    }
    return read_block(1, 1, s_block, sizeof(struct ext2_super_block));
}

static int read_b_groups(void)
{
    size_t size;
    uint32_t blocks;

    groups_count = ALIGN_CEIL(s_block->blocks_count, s_block->blocks_per_group)/s_block->blocks_per_group;
    size = sizeof(struct ext2_block_group) * groups_count;
    b_groups = kmalloc(size);
    if (b_groups == NULL) {
        return -ENOMEM;
    }
    blocks = ceil_div(size, CONFIG_EXT2_BS);
    return read_block(2, blocks, b_groups, size);
}

static uint32_t search_dir_L0(uint32_t dir_ino, const char *name)
{
    int ret;
    struct ext2_dir_entry *dir_ent;
    char buf[CONFIG_EXT2_BS];

    ret = read_block(dir_ino, 1, buf, CONFIG_EXT2_BS);
    if (ret != 0) {
        log_error("Ext2: read block error, errno: %d\n", ret);
        return 0;
    }
    dir_ent = (struct ext2_dir_entry *)buf;
    while ((void *)dir_ent < (void *)buf + CONFIG_EXT2_BS &&
           dir_ent->inode != 0 && strncmp(dir_ent->name, name, dir_ent->name_len) != 0) {
        dir_ent = (void *)dir_ent + dir_ent->rec_len;
    }
    if ((void *)dir_ent == (void *)buf + CONFIG_EXT2_BS || dir_ent->inode == 0) {
        return 0;
    }
    return dir_ent->inode;
}

static uint32_t search_dir_L1(uint32_t dir_ino, const char *name)
{
    int i, ret;
    uint32_t m, ino;
    char buf[CONFIG_EXT2_BS];

    ret = read_block(dir_ino, 1, buf, CONFIG_EXT2_BS);
    if (ret != 0) {
        log_error("Ext2: read block error, errno: %d\n", ret);
        return 0;
    }
    for (i = 0; i < CONFIG_EXT2_BS/sizeof(uint32_t); i++) {
        m = *(uint32_t *)((void *)buf + i * sizeof(uint32_t));
        if (m == 0) {
            break;
        }
        ino = search_dir_L0(m, name);
        if (ino != 0) {
            return ino;
        }
    }
    return 0;
}

static uint32_t search_dir_L2(uint32_t dir_ino, const char *name)
{
    int i, ret;
    uint32_t m, ino;
    char buf[CONFIG_EXT2_BS];

    ret = read_block(dir_ino, 1, buf, CONFIG_EXT2_BS);
    if (ret != 0) {
        log_error("Ext2: read block error, errno: %d\n", ret);
        return 0;
    }
    for (i = 0; i < CONFIG_EXT2_BS/sizeof(uint32_t); i++) {
        m = *(uint32_t *)((void *)buf + i * sizeof(uint32_t));
        if (m == 0) {
            break;
        }
        ino = search_dir_L1(m, name);
        if (ino != 0) {
            return ino;
        }
    }
    return 0;
}

static uint32_t search_dir_L3(uint32_t dir_ino, const char *name)
{
    int i, ret;
    uint32_t m, ino;
    char buf[CONFIG_EXT2_BS];

    ret = read_block(dir_ino, 1, buf, CONFIG_EXT2_BS);
    if (ret != 0) {
        log_error("Ext2: read block error, errno: %d\n", ret);
        return 0;
    }
    for (i = 0; i < CONFIG_EXT2_BS/sizeof(uint32_t); i++) {
        m = *(uint32_t *)((void *)buf + i * sizeof(uint32_t));
        if (m == 0) {
            break;
        }
        ino = search_dir_L2(m, name);
        if (ino != 0) {
            return ino;
        }
    }
    return 0;
}

static uint32_t search_in_dir(uint32_t dir_ino, const char *name)
{
    int i, ret;
    uint32_t ino;
    struct ext2_inode inode;

    ret = read_inode(dir_ino, &inode);
    if (ret != 0) {
        log_error("read inode error, errno: %d\n", ret);
        return 0;
    }
    for (i = 0; i < EXT2_N_BLOCKS && inode.block[i] != 0; i++) {
        if (i < EXT2_BLOCK_L1_INDEX) {
            ino = search_dir_L0(inode.block[i], name);
        } else if (i == EXT2_BLOCK_L1_INDEX) {
            ino = search_dir_L1(inode.block[i], name);
        } else if (i == EXT2_BLOCK_L2_INDEX) {
            ino = search_dir_L2(inode.block[i], name);
        } else if (i == EXT2_BLOCK_L3_INDEX) {
            ino = search_dir_L3(inode.block[i], name);
        } else {
            kernel_panic("Ext2: NEVER REACH!!!\n");
        }

        // found
        if (ino != 0) {
            return ino;
        }
    }

    return 0;
}

static int read_L0(uint32_t ino, void *buf, size_t size)
{
    int ret;

    ret = read_block(ino, 1, buf, size);
    if (ret != 0) {
        log_error("Ext2: read block error, errno: %d\n", ret);
        return ret;
    }
    return min(CONFIG_EXT2_BS, size);
}

static int read_L1(uint32_t ino, void *buf, size_t size)
{
    int i, ret;
    uint32_t m, offset;
    uint8_t block_table[CONFIG_EXT2_BS];

    offset = 0;
    ret = read_block(ino, 1, block_table, CONFIG_EXT2_BS);
    if (ret != 0) {
        log_error("Ext2: read block error, errno: %d\n", ret);
        return ret;
    }
    for (i = 0; i < CONFIG_EXT2_BS/sizeof(uint32_t); i++) {
        m = *(uint32_t *)((void *)block_table + i * sizeof(uint32_t));
        if (m == 0) {
            break;
        }
        ret = read_L0(m, buf+offset, size-offset);
        if (ret < 0) {
            return ret;
        }
        offset += ret;
    }
    return offset;
}

static int read_L2(uint32_t ino, void *buf, size_t size)
{
    int i, ret;
    uint32_t m, offset;
    uint8_t block_table[CONFIG_EXT2_BS];

    offset = 0;
    ret = read_block(ino, 1, block_table, CONFIG_EXT2_BS);
    if (ret != 0) {
        log_error("Ext2: read block error, errno: %d\n", ret);
        return ret;
    }
    for (i = 0; i < CONFIG_EXT2_BS/sizeof(uint32_t); i++) {
        m = *(uint32_t *)((void *)block_table + i * sizeof(uint32_t));
        if (m == 0) {
            break;
        }
        ret = read_L1(m, buf+offset, size-offset);
        if (ret < 0) {
            return ret;
        }
        offset += ret;
    }
    return offset;
}

static int read_L3(uint32_t ino, void *buf, size_t size)
{
    int i, ret;
    uint32_t m, offset;
    uint8_t block_table[CONFIG_EXT2_BS];

    offset = 0;
    ret = read_block(ino, 1, block_table, CONFIG_EXT2_BS);
    if (ret != 0) {
        log_error("Ext2: read block error, errno: %d\n", ret);
        return ret;
    }
    for (i = 0; i < CONFIG_EXT2_BS/sizeof(uint32_t); i++) {
        m = *(uint32_t *)((void *)block_table + i * sizeof(uint32_t));
        if (m == 0) {
            break;
        }
        ret = read_L2(m, buf+offset, size-offset);
        if (ret < 0) {
            return ret;
        }
        offset += ret;
    }
    return offset;
}

static ssize_t ext2_f_read(struct file *f, void *buf, size_t count)
{
    int i, ret;
    struct ext2_inode *inode = f->dentry->inode->priv;

    uint32_t offset = 0;
    for (i = 0; i < EXT2_N_BLOCKS && inode->block[i] != 0; i++) {
        if (i < EXT2_BLOCK_L1_INDEX) { // TODO
            ret = read_L0(inode->block[i], buf+offset, count-offset);
        } else if (i == EXT2_BLOCK_L1_INDEX) {
            ret = read_L1(inode->block[i], buf+offset, count-offset);
        } else if (i == EXT2_BLOCK_L2_INDEX) {
            ret = read_L2(inode->block[i], buf+offset, count-offset);
        } else if (i == EXT2_BLOCK_L3_INDEX) {
            ret = read_L3(inode->block[i], buf+offset, count-offset);
        } else {
            kernel_panic("Ext2: NEVER REACH!!!\n");
        }

        if (ret < 0) {
            return ret;
        }
        offset += ret;
    }
    return offset;
}

static ssize_t ext2_f_write(struct file *f, const void *buf, size_t count)
{
    return 0;
}

static int ext2_f_close(struct file *f)
{
    kfree(f->dentry->inode->priv);
    return 0;
}

int ext2_inode_create(struct inode *dir, struct dentry *dentry, int mode)
{
    return 0;
}

static int ext2_inode_lookup(struct inode *dir, struct dentry *dentry)
{
    uint32_t ino = search_in_dir(dir->ino, dentry->name);
    if (ino == 0) {
        return -ENOENT;
    }

    struct ext2_inode *ext2_ino = kmalloc(sizeof(struct ext2_inode));
    if (ext2_ino == NULL) {
        return -ENOMEM;
    }

    int ret = read_inode(ino, ext2_ino);
    if (ret != 0) {
        kfree(ext2_ino);
        return -EIO;
    }

    struct inode *inode = kmalloc(sizeof(struct inode));
    if (inode == NULL) {
        kfree(ext2_ino);
        return -ENOMEM;
    }

    inode->ino = ino;
    inode->size = ext2_ino->size;
    inode->ops = &ext2_inode_ops;
    inode->priv = ext2_ino;
    inode->dentry.prev = &inode->dentry;
    inode->dentry.next = &inode->dentry;
    LIST_ADD(&inode->dentry, &dentry->alias);
    dentry->inode = inode;
    return 0;
}

void ext2_mount_rootfs(struct fs *fs)
{
    struct ext2_inode inode;
    int ret = read_inode(EXT2_INO_ROOT, &inode);
    if (ret != 0) {
        kernel_panic("read root inode error, code: %d\n", ret);
    }

    struct mount *mnt = kmalloc(sizeof(struct mount));
    if (mnt == NULL) {
        kernel_panic("no memory for mount\n");
    }
    mnt->fs = fs;
    mnt->dentry = kmalloc(sizeof(struct dentry));
    if (mnt->dentry == NULL) {
        kfree(mnt);
        kernel_panic("no memory for root dentry\n");
    }
    dentry_init(mnt->dentry);
    mnt->dentry->rc = 1;
    mnt->dentry->inode = kmalloc(sizeof(struct inode));
    if (mnt->dentry->inode == NULL) {
        kfree(mnt->dentry);
        kfree(mnt);
        kernel_panic("no memory for root inode\n");
    }
    mnt->dentry->inode->ino = EXT2_INO_ROOT;
    mnt->dentry->inode->size = inode.size;
    mnt->dentry->inode->ops = &ext2_inode_ops;
    LIST_HEAD_INIT(&mnt->dentry->inode->dentry);
    LIST_ADD(&mnt->dentry->inode->dentry, &mnt->dentry->alias);

    ret = vfs_mount("/", mnt);
    if (ret != 0) {
        kfree(mnt->dentry);
        kfree(mnt);
        kernel_panic("mount rootfs error, code: %d\n", ret);
    }
    log_info("root fs mounted.\n");
}

void ext2_setup(void)
{
    int ret;
    static struct fs ext2_fs = {
        .name = "EXT2",
        .ops = {},
        .f_ops = {
            .read = ext2_f_read,
            .write = ext2_f_write,
            .close = ext2_f_close,
        },
    };

    log_info("setup ext2 fs\n");
    ret = read_s_block();
    if (ret != 0) {
        kernel_panic("read s_block error, code: %d\n", ret);
    }
    ret = read_b_groups();
    if (ret != 0) {
        kernel_panic("read block groups error, code: %d\n", ret);
    }
    log_info("ext2 fs has %u blocks in %u groups\n", s_block->blocks_count, groups_count);

    ext2_mount_rootfs(&ext2_fs);
}

static struct inode_ops ext2_inode_ops = {
    .create = ext2_inode_create,
    .lookup = ext2_inode_lookup,
};