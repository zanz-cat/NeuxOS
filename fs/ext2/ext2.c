#include <string.h>

#include <neuxos.h>
#include <errcode.h>
#include <kernel/log.h>
#include <kernel/printk.h>
#include <kernel/interrupt.h>
#include <kernel/kernel.h>
#include <kernel/kmalloc.h>
#include <drivers/harddisk.h>
#include <lib/list.h>
#include <misc/misc.h>

#include "ext2.h"

#define CACHE_SIZE 1000

static uint32_t part_lba;
static struct ext2_super_block *s_block;
static struct ext2_block_group *b_groups;
static uint32_t groups_count;

static uint32_t inodes_per_block = CONFIG_EXT2_BS/sizeof(struct ext2_inode);

static LIST_HEAD(inode_cache);

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
        return -EINTVAL;
    }
    for (i = 0; i < HD_MBR_DPT_SIZE; i++) {
        if (mbr.DPT[i].boot == PART_BOOTABLE) {
            partition = &mbr.DPT[i];
        }
    }
    if (partition == NULL) {
        return -EERR;
    }
    part_lba = partition->lba;

    s_block = kmalloc(sizeof(struct ext2_super_block));
    if (s_block == NULL) {
        return -EOOM;
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
        return -EOOM;
    }
    blocks = ceil_div(size, CONFIG_EXT2_BS);
    return read_block(2, blocks, b_groups, size);
}

void ext2_setup(void)
{
    int ret;

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

struct ext2_file *ext2_open(const char *abspath)
{
    int ret;
    char *s, *name;
    struct ext2_file *f;
    struct ext2_inode inode;
    uint32_t ino = EXT2_INO_ROOT;
    char buf[MAX_PATH_LEN];

    strncpy(buf, abspath, MAX_PATH_LEN);
    s = buf;
    while ((name = strsep(&s, FILE_PATH_SEP)) != NULL) {
        if (strlen(name) == 0) {
            continue;
        }
        ino = search_in_dir(ino, name);
        if (ino == 0) {
            break;
        }
    }
    if (ino == 0) {
        return NULL;
    }
    ret = read_inode(ino, &inode);
    if (ret != 0) {
        return NULL;
    }
    f = kmalloc(sizeof(struct ext2_file));
    if (f == NULL) {
        return NULL;
    }
    f->inode = ino;
    f->mode = inode.mode;
    f->size = inode.size;
    return f;
}

int ext2_close(struct ext2_file *f)
{
    kfree(f);
    return 0;
}

static int read_L0(uint32_t ino, void *buf)
{
    int ret;

    ret = read_block(ino, 1, buf, CONFIG_EXT2_BS);
    if (ret != 0) {
        log_error("Ext2: read block error, errno: %d\n", ret);
        return ret;
    }
    return CONFIG_EXT2_BS;
}

static int read_L1(uint32_t ino, void *buf)
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
        ret = read_L0(m, buf+offset);
        if (ret < 0) {
            return ret;
        }
        offset += ret;
    }
    return offset;
}

static int read_L2(uint32_t ino, void *buf)
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
        ret = read_L1(m, buf+offset);
        if (ret < 0) {
            return ret;
        }
        offset += ret;
    }
    return offset;
}

static int read_L3(uint32_t ino, void *buf)
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
        ret = read_L2(m, buf+offset);
        if (ret < 0) {
            return ret;
        }
        offset += ret;
    }
    return offset;
}

int ext2_read(struct ext2_file *f, void *buf, size_t size)
{
    int i, ret;
    uint32_t offset;
    struct ext2_inode inode;

    if (size < f->size) {
        return -EOVERFLOW;
    }

    offset = 0;
    ret = read_inode(f->inode, &inode);
    if (ret != 0) {
        log_error("Ext2: read block error, errno: %d\n", ret);
        return ret;
    }
    for (i = 0; i < EXT2_N_BLOCKS && inode.block[i] != 0; i++) {
        if (i < EXT2_BLOCK_L1_INDEX) {
            ret = read_L0(inode.block[i], buf+offset);
        } else if (i == EXT2_BLOCK_L1_INDEX) {
            ret = read_L1(inode.block[i], buf+offset);
        } else if (i == EXT2_BLOCK_L2_INDEX) {
            ret = read_L2(inode.block[i], buf+offset);
        } else if (i == EXT2_BLOCK_L3_INDEX) {
            ret = read_L3(inode.block[i], buf+offset);
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

int ext2_write(struct ext2_file *f, const void *buf, size_t count)
{
    return 0;
}