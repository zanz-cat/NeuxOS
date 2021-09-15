#ifndef __FS_EXT2_H__
#define __FS_EXT2_H__

#include <stdint.h>
#include <config.h>

#define EXT2_N_BLOCKS 15
#define EXT2_BLOCK_L1_INDEX 12
#define EXT2_BLOCK_L2_INDEX 13
#define EXT2_BLOCK_L3_INDEX 14

#define EXT2_MAX_NAME_LEN 255

#define EXT2_FT_UNKNOWN 0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR 2
#define EXT2_FT_CHRDEV 3
#define EXT2_FT_BLKDEV 4
#define EXT2_FT_FIFO 5
#define EXT2_FT_SOCK 6
#define EXT2_FT_SYMLINK 7

#define EXT2_SECTS_PER_BLK (CONFIG_EXT2_BS / CONFIG_HD_SECT_SZ)

// inode number start from 1 rather than 0
#define EXT2_INO_ROOT 2

struct ext2_super_block {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t reserved_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mtime;
    uint32_t wtime;
    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_rev_level;
    uint32_t last_check;
    uint32_t check_interval;
    uint32_t creator_os;
    uint32_t rev_level;
    uint16_t def_resuid;
    uint16_t def_resgid;
    uint32_t first_ino;
    uint16_t inode_size;
    uint16_t block_group_nr;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
    uint8_t uuid[16];
    char volume_name[16];
    char last_mnt[64];
    uint32_t algorithm_usage_bitmap;
    uint8_t prealloc_blocks;
    uint8_t prealloc_dir_blocks;
    uint16_t padding1;
    uint32_t reserved[204];
} __attribute__((packed));

#define EXT2_S_BLOCK_BS(block_size) (1024 << (block_size))

struct ext2_block_group {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint16_t padding;
    uint32_t reserved[3];
} __attribute__((packed));

struct ext2_inode {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t blocks;
    uint32_t frags;
    uint8_t osd1[4];
    uint32_t block[EXT2_N_BLOCKS];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t frag_addr;
    uint8_t osd2[12];
} __attribute__((packed));

struct ext2_dir_entry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[0];
} __attribute__((packed));

struct ext2_index_dir_entry {

} __attribute__((packed));

#define FILE_PATH_SEP "/"

struct ext2_file {
    uint32_t inode;
    uint32_t size;
    uint16_t mode;
};

void ext2_setup(void);
struct ext2_file *ext2_open(const char *abspath);
int ext2_close(struct ext2_file *f);
int ext2_read(struct ext2_file *f, void *buf, size_t count);
int ext2_write(struct ext2_file *f, const void *buf, size_t count);

#endif