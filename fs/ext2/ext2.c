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
#define INO_CNT_PB (CONFIG_EXT2_BS/sizeof(uint32_t))

struct block_iter {
    const struct ext2_inode *inode;
    uint32_t *L1;
    uint32_t *L2;
    uint32_t *L3;
    int index;
    int L2i;
    int L3i;
};

static uint32_t part_lba;
static struct ext2_super_block *s_block;
static struct ext2_block_group *b_groups;
static uint32_t groups_count;
static uint32_t inodes_per_block = CONFIG_EXT2_BS/sizeof(struct ext2_inode);
static LIST_HEAD(inode_cache);
static struct inode_ops ext2_inode_ops;

static inline int read_block(uint32_t block, uint32_t count, void *buf, size_t size)
{
    int ret;

    ret = hd_read(part_lba+(CONFIG_EXT2_BS/CONFIG_HD_SECT_SZ)*block, 
                  count*CONFIG_EXT2_BS/CONFIG_HD_SECT_SZ, buf, size);
    if (ret < 0) {
        log_debug("Ext2: read block error, errno: %d\n", ret);
    }
    return ret;
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
    if (ret < 0) {
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
    if (ret < 0) {
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

static inline void block_iter_init(struct block_iter *iter, const struct ext2_inode *inode)
{
    iter->inode = inode;
    iter->L1 = NULL;
    iter->L2 = NULL;
    iter->L3 = NULL;
    iter->index = 0;
    iter->L2i = -1;
    iter->L3i = -1;
}

static inline void block_iter_destroy(struct block_iter *iter)
{
    if (iter->L1 != NULL) {
        kfree(iter->L1);
    }
    if (iter->L2 != NULL) {
        kfree(iter->L2);
    }
    if (iter->L3 != NULL) {
        kfree(iter->L3);
    }
}

static int block_iter_ld_L1(struct block_iter *iter, uint32_t ino)
{
    iter->L1 = kmalloc(CONFIG_EXT2_BS);
    if (iter->L1 == NULL) {
        return -ENOMEM;
    }
    return read_block(ino, 1, iter->L1, CONFIG_EXT2_BS);
}

static int block_iter_L1(struct block_iter *iter, void *buf, size_t count)
{
    int res, L1i;

    if (iter->inode->block[EXT2_B_L1_IDX] == 0) {
        return 0;
    }
    if (iter->L1 == NULL) {
        res = block_iter_ld_L1(iter, iter->inode->block[EXT2_B_L1_IDX]);
        if (res < 0) {
            return res;
        }
    }
    L1i = iter->index - EXT2_B_L1_IDX;
    if (iter->L1[L1i] == 0) {
        return 0;
    }
    return read_block(iter->L1[L1i], 1, buf, count);
}

static int block_iter_ld_L2(struct block_iter *iter, uint32_t ino)
{
    iter->L2 = kmalloc(CONFIG_EXT2_BS);
    if (iter->L2 == NULL) {
        return -ENOMEM;
    }
    return read_block(ino, 1, iter->L2, CONFIG_EXT2_BS);
}

static int block_iter_L2(struct block_iter *iter, void *buf, size_t count)
{
    int res, L2i, L1i;

    if (iter->inode->block[EXT2_B_L2_IDX] == 0) {
        return 0;
    }
    if (iter->L2 == NULL) {
        res = block_iter_ld_L2(iter, iter->inode->block[EXT2_B_L2_IDX]);
        if (res < 0) {
            return res;
        }
    }
    L2i = (iter->index - EXT2_B_L1_IDX) / INO_CNT_PB;
    if (iter->L2i != L2i) {
        if (iter->L2[L2i] == 0) {
            return 0;
        }
        res = block_iter_ld_L1(iter, iter->L2[L2i]);
        if (res < 0) {
            return res;
        }
        iter->L2i = L2i;
    }
    L1i = (iter->index - EXT2_B_L1_IDX) % INO_CNT_PB;
    if (iter->L1[L1i] == 0) {
        return 0;
    }
    return read_block(iter->L1[L1i], 1, buf, count);
}

static int block_iter_ld_L3(struct block_iter *iter, uint32_t ino)
{
    iter->L3 = kmalloc(CONFIG_EXT2_BS);
    if (iter->L3 == NULL) {
        return -ENOMEM;
    }
    return read_block(ino, 1, iter->L3, CONFIG_EXT2_BS);
}

static int block_iter_L3(struct block_iter *iter, void *buf, size_t count)
{
    int res, L3i, L2i, L1i;

    if (iter->inode->block[EXT2_B_L3_IDX] == 0) {
        return 0;
    }
    if (iter->L3 == NULL) {
        res = block_iter_ld_L3(iter, iter->inode->block[EXT2_B_L3_IDX]);
        if (res < 0) {
            return res;
        }
    }
    L3i = (iter->index - EXT2_B_L1_IDX) / (INO_CNT_PB * INO_CNT_PB);
    if (iter->L3i != L3i) {
        if (iter->L3[L3i] == 0) {
            return 0;
        }
        res = block_iter_ld_L2(iter, iter->L3[L3i]);
        if (res < 0) {
            return res;
        }
        iter->L3i = L3i;
    }
    L2i = (iter->index - EXT2_B_L1_IDX) % (INO_CNT_PB * INO_CNT_PB) / INO_CNT_PB;
    if (iter->L2i != L2i) {
        if (iter->L2[L2i] == 0) {
            return 0;
        }
        res = block_iter_ld_L1(iter, iter->L2[L2i]);
        if (res < 0) {
            return res;
        }
        iter->L2i = L2i;
    }
    L1i = (iter->index - EXT2_B_L1_IDX) % (INO_CNT_PB * INO_CNT_PB) % INO_CNT_PB;
    if (iter->L1[L1i] == 0) {
        return 0;
    }
    return read_block(iter->L1[L1i], 1, buf, count);
}

static int block_iter_next(struct block_iter *iter, void *buf, size_t count)
{
    int res;

    if (iter->index < EXT2_B_L1_IDX) {
        if (iter->inode->block[iter->index] == 0) {
            return 0;
        }
        res = read_block(iter->inode->block[iter->index], 1, buf, count);
    } else if (iter->index < EXT2_B_L1_IDX + INO_CNT_PB) {
        res = block_iter_L1(iter, buf, count);
    } else if (iter->index < EXT2_B_L1_IDX + INO_CNT_PB * INO_CNT_PB) {
        res = block_iter_L2(iter, buf, count);
    } else if (iter->index < EXT2_B_L1_IDX + INO_CNT_PB * INO_CNT_PB * INO_CNT_PB) {
        res = block_iter_L3(iter, buf, count);
    } else {
        log_error("EXT2: NEVER REACH!!!\n");
        return -EINVAL;
    }
    iter->index++;
    return res;
}

static uint32_t search_in_dir(uint32_t dir_ino, const char *name)
{
    int ret;
    uint32_t ino = 0;
    struct ext2_inode inode;
    char buf[CONFIG_EXT2_BS];
    struct ext2_dir_entry *dir_ent;
    struct block_iter iter;

    ret = read_inode(dir_ino, &inode);
    if (ret != 0) {
        log_error("read inode error, errno: %d\n", ret);
        return 0;
    }

    block_iter_init(&iter, &inode);
    while (block_iter_next(&iter, buf, CONFIG_EXT2_BS) > 0) {
        dir_ent = (struct ext2_dir_entry *)buf;
        while ((void *)dir_ent < (void *)buf + CONFIG_EXT2_BS &&
            dir_ent->inode != 0 && strncmp(dir_ent->name, name, dir_ent->name_len) != 0) {
            dir_ent = (void *)dir_ent + dir_ent->rec_len;
        }
        if ((void *)dir_ent == (void *)buf + CONFIG_EXT2_BS || dir_ent->inode == 0) {
            break;
        }
        ino = dir_ent->inode;
        break;
    }
    block_iter_destroy(&iter);
    return ino;
}

static ssize_t ext2_f_read(struct file *f, void *buf, size_t count)
{
    int ret;
    size_t offset = 0;
    struct block_iter iter;

    block_iter_init(&iter, (struct ext2_inode *)f->dentry->inode->priv);
    while ((ret = block_iter_next(&iter, buf + offset, count - offset)) > 0) {
        offset += ret;
    }
    block_iter_destroy(&iter);
    return ret < 0 ? ret : offset;
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
    if (ret < 0) {
        kernel_panic("read s_block error, code: %d\n", ret);
    }
    ret = read_b_groups();
    if (ret < 0) {
        kernel_panic("read block groups error, code: %d\n", ret);
    }
    log_info("ext2 fs has %u blocks in %u groups\n", s_block->blocks_count, groups_count);

    ext2_mount_rootfs(&ext2_fs);
}

static struct inode_ops ext2_inode_ops = {
    .create = ext2_inode_create,
    .lookup = ext2_inode_lookup,
};