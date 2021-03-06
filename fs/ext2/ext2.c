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
#include <lib/misc.h>
#include <fs/fs.h>

#include "ext2.h"

#define CACHE_SIZE 1000
#define INO_CNT_PB (CONFIG_EXT2_BS/sizeof(uint32_t))

struct block_iter {
    const struct ext2_inode *inode;
    uint32_t index;
    uint32_t *L1;
    uint32_t *L2;
    uint32_t *L3;
    uint32_t L1i;
    uint32_t L2i;
    uint32_t L3i;
};

static uint32_t part_lba;
static struct ext2_super_block *s_block;
static struct ext2_block_group *b_groups;
static uint32_t groups_count;
static uint32_t inodes_per_block = CONFIG_EXT2_BS/sizeof(struct ext2_inode);
static LIST_HEAD(inode_cache);

static int ext2_i_create(struct inode *dir, struct dentry *dentry, int mode);
static int ext2_i_lookup(struct inode *dir, struct dentry *dentry);
static void ext2_i_release(struct inode *inode);

static ssize_t ext2_f_read(struct file *pfile, void *buf, size_t count);
static int ext2_f_readdir(struct file *pfile, struct dirent *dent);
static ssize_t ext2_f_write(struct file *pfile, const void *buf, size_t count);
static int ext2_f_close(struct file *pfile);

static struct fs ext2_fs = {
    .name = "ext2",
    .ops = {},
    .i_ops = {
        .create = ext2_i_create,
        .lookup = ext2_i_lookup,
        .release = ext2_i_release,
    },
    .f_ops = {
        .read = ext2_f_read,
        .readdir = ext2_f_readdir,
        .write = ext2_f_write,
        .close = ext2_f_close,
    },
};

static inline int read_block(uint32_t block, uint32_t count, void *buf, size_t size)
{
    int ret;

    ret = hd_read(part_lba+(CONFIG_EXT2_BS/CONFIG_HD_SECT_SZ)*block, 
                  count*CONFIG_EXT2_BS/CONFIG_HD_SECT_SZ, buf, size);
    if (ret < 0) {
        log_debug("[ext2] read block error, errno: %d\n", ret);
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
    iter->index = 0;
    iter->L1i = 0;
    iter->L2i = 0;
    iter->L3i = 0;
    iter->L1 = NULL;
    iter->L2 = NULL;
    iter->L3 = NULL;
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

static int block_iter_L1(struct block_iter *iter,
                         uint32_t idx_ino, uint32_t offset,
                         void *buf, size_t count)
{
    int res;

    if (idx_ino == 0) {
        return 0;
    }
    if (iter->L1i != idx_ino) {
        if (iter->L1 == NULL) {
            iter->L1 = kmalloc(CONFIG_EXT2_BS);
            if (iter->L1 == NULL) {
                return -ENOMEM;
            }
        }
        res = read_block(idx_ino, 1, iter->L1, CONFIG_EXT2_BS);
        if (res < 0) {
            return res;
        }
    }
    if (iter->L1[offset] == 0) {
        return 0;
    }
    return read_block(iter->L1[offset], 1, buf, count);
}

static int block_iter_L2(struct block_iter *iter,
                         uint32_t idx_ino, uint32_t offset,
                         void *buf, size_t count)
{
    int res;

    if (idx_ino == 0) {
        return 0;
    }
    if (iter->L2i != idx_ino) {
        if (iter->L2 == NULL) {
            iter->L2 = kmalloc(CONFIG_EXT2_BS);
            if (iter->L2 == NULL) {
                return -ENOMEM;
            }
        }
        res = read_block(idx_ino, 1, iter->L2, CONFIG_EXT2_BS);
        if (res < 0) {
            return res;
        }
    }
    return block_iter_L1(iter, offset / INO_CNT_PB, offset % INO_CNT_PB, buf, count);
}

static int block_iter_L3(struct block_iter *iter,
                         uint32_t idx_ino, uint32_t offset,
                         void *buf, size_t count)
{
    int res;

    if (idx_ino == 0) {
        return 0;
    }
    if (iter->L3i != idx_ino) {
        if (iter->L3 == NULL) {
            iter->L3 = kmalloc(CONFIG_EXT2_BS);
            if (iter->L3 == NULL) {
                return -ENOMEM;
            }
        }
        res = read_block(idx_ino, 1, iter->L3, CONFIG_EXT2_BS);
        if (res < 0) {
            return res;
        }
    }
    return block_iter_L2(iter, offset / (INO_CNT_PB * INO_CNT_PB),
                         offset % (INO_CNT_PB * INO_CNT_PB), buf, count);
}

static int block_read_iter(struct block_iter *iter, void *buf, size_t count)
{
    int res;

    if (iter->index < EXT2_B_L1_IDX) {
        if (iter->inode->block[iter->index] == 0) {
            return 0;
        }
        res = read_block(iter->inode->block[iter->index], 1, buf, count);
    } else if (iter->index < EXT2_B_L1_IDX + INO_CNT_PB) {
        res = block_iter_L1(iter, iter->inode->block[EXT2_B_L1_IDX], 
                            iter->index - EXT2_B_L1_IDX, buf, count);
    } else if (iter->index < EXT2_B_L1_IDX + INO_CNT_PB * INO_CNT_PB) {
        res = block_iter_L2(iter, iter->inode->block[EXT2_B_L2_IDX],
                            iter->index - (EXT2_B_L1_IDX + INO_CNT_PB),
                            buf, count);
    } else if (iter->index < EXT2_B_L1_IDX + INO_CNT_PB * INO_CNT_PB * INO_CNT_PB) {
        res = block_iter_L3(iter, iter->inode->block[EXT2_B_L3_IDX],
                            iter->index - (EXT2_B_L1_IDX + INO_CNT_PB * INO_CNT_PB),
                            buf, count);
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
    size_t len;
    uint32_t ino = 0;
    struct ext2_inode inode;
    char buf[CONFIG_EXT2_BS];
    struct ext2_dir_entry *dirent;
    struct block_iter iter;

    ret = read_inode(dir_ino, &inode);
    if (ret != 0) {
        log_error("read inode error, errno: %d\n", ret);
        return 0;
    }

    len = strlen(name);
    block_iter_init(&iter, &inode);
    while (block_read_iter(&iter, buf, CONFIG_EXT2_BS) > 0) {
        dirent = (struct ext2_dir_entry *)buf;
        while ((void *)dirent < (void *)buf + CONFIG_EXT2_BS && dirent->inode != 0 &&
            !(dirent->name_len == len && strncmp(dirent->name, name, len) == 0)) {
            dirent = (void *)dirent + dirent->rec_len;
        }
        if ((void *)dirent == (void *)buf + CONFIG_EXT2_BS || dirent->inode == 0) {
            break;
        }
        ino = dirent->inode;
        break;
    }
    block_iter_destroy(&iter);
    return ino;
}

static ssize_t ext2_f_read(struct file *pfile, void *buf, size_t count)
{
    int ret;
    size_t offset = 0;
    struct block_iter iter;

    block_iter_init(&iter, (struct ext2_inode *)(pfile->dent->inode->priv));
    while ((ret = block_read_iter(&iter, buf + offset, count - offset)) > 0) {
        offset += ret;
    }
    block_iter_destroy(&iter);
    return ret < 0 ? ret : offset;
}

static ssize_t ext2_f_write(struct file *pfile, const void *buf, size_t count)
{
    return 0;
}

static int ext2_f_close(struct file *pfile)
{
    kfree(pfile->buf);
    return 0;
}

static int ext2_f_readdir(struct file *pfile, struct dirent *dent)
{
    int ret;
    struct inode *pinode = pfile->dent->inode;
    struct ext2_dir_entry *ext2_dent;
    
    if (pfile->buf == NULL) {
        pfile->buf = kmalloc(pinode->size);
        if (pfile->buf == NULL) {
            return -ENOMEM;
        }
        ret = ext2_f_read(pfile, pfile->buf, pinode->size);
        if (ret < 0) {
            return ret;
        }
    }
    if (pfile->off >= pinode->size) {
        return -EOF;
    }

    ext2_dent = pfile->buf + pfile->off;
    dent->d_ino = ext2_dent->inode;
    dent->d_off = 0;
    dent->d_type = ext2_dent->file_type;
    dent->d_reclen = ext2_dent->rec_len;
    if (ext2_dent->name_len > MAX_PATH_LEN) {
        return -EINVAL;
    }
    strncpy(dent->d_name, ext2_dent->name, ext2_dent->name_len);
    dent->d_name[ext2_dent->name_len] = '\0';
    pfile->off += ext2_dent->rec_len;

    return 0;
}

static int ext2_i_create(struct inode *dir, struct dentry *dent, int mode)
{
    return 0;
}

static int ext2_i_lookup(struct inode *dir, struct dentry *dent)
{
    uint32_t ino = search_in_dir(dir->ino, dent->name);
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
    inode->mode = ext2_ino->mode;
    inode->size = ext2_ino->size;
    inode->ops = &ext2_fs.i_ops;
    inode->priv = ext2_ino;
    inode->dent.prev = &inode->dent;
    inode->dent.next = &inode->dent;
    LIST_ADD(&inode->dent, &dent->alias);
    dent->inode = inode;
    return 0;
}

static void ext2_i_release(struct inode *inode)
{
    kfree(inode->priv);
    kfree(inode);
}

void ext2_mount_rootfs(struct fs *fs)
{
    int ret;
    const char *err = NULL;
    struct mount *mnt = NULL;
    struct ext2_inode *ext2_inode = NULL;

    ext2_inode = kmalloc(sizeof(struct ext2_inode));
    if (ext2_inode == NULL) {
        err = "no memory ext2_inode";
        goto panic;
    }

    ret = read_inode(EXT2_INO_ROOT, ext2_inode);
    if (ret != 0) {
        err = "read root inode error";
        goto panic;
    }

    mnt = kmalloc(sizeof(struct mount));
    if (mnt == NULL) {
        err = "no memory for mount";
        goto panic;
    }
    mnt->fs = fs;
    mnt->dent = kmalloc(sizeof(struct dentry));
    if (mnt->dent == NULL) {
        err = "no memory for root dentry";
        goto panic;
    }
    dentry_init(mnt->dent);
    mnt->dent->rc = 1;
    mnt->dent->mnt = mnt;
    mnt->dent->inode = kmalloc(sizeof(struct inode));
    if (mnt->dent->inode == NULL) {
        err = "no memory for root inode";
        goto panic;
    }
    inode_init(mnt->dent->inode, EXT2_INO_ROOT, ext2_inode->mode,
               ext2_inode->size, &fs->i_ops);
    mnt->dent->inode->priv = ext2_inode;
    LIST_ADD(&mnt->dent->inode->dent, &mnt->dent->alias);

    ret = vfs_mount("/", mnt);
    if (ret != 0) {
        err = "mount rootfs error";
        goto panic;
    }
    log_info("rootfs mounted.\n");
    return;

panic:
    if (mnt != NULL) {
        if (mnt->dent != NULL) {
            kfree(mnt->dent->inode);
        }
        kfree(mnt->dent);
    }
    kfree(mnt);
    kfree(ext2_inode);
    kernel_panic("err=%s, code=%d\n", err, ret);
}

void ext2_setup(void)
{
    int ret;

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