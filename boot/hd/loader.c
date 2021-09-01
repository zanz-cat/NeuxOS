#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <elf.h>

#include <drivers/io.h>
#include <drivers/harddisk/harddisk.h>
#include <drivers/monitor/monitor.h>

#include <lib/utils.h>
#include <fs/ext2/ext2.h>
#include <errcode.h>
#include <config.h>

#define MEMORY_AVL 1U
#define MEMORY_RES 2U

#define WAIT_HD_TIMES 10000
#define ARDS_ADDR ((void *)0x60000)
#define KERNEL_FILE_NAME "KERNEL.ELF"

#define ERR_READ_MBR 0x100
#define ERR_NO_BOOTABLE_PART 0x101
#define ERR_INVALID_FS 0x102
#define ERR_READ_EXT2_S_BLOCK 0x103
#define ERR_READ_EXT2_BLOCK_GROUP 0x104
#define ERR_READ_EXT2_INODE_MAP 0x105
#define ERR_NO_ROOT_DIR 0x106
#define ERR_READ_EXT2_INODE_TABLE 0x107
#define ERR_READ_EXT2_ROOT_DIR 0x108
#define ERR_NO_KERNEL_FILE 0x109
#define ERR_READ_KERNEL_BLOCK 0x110
#define ERR_NON_ELF 0x111

struct ARDS {
    uint16_t count;
    struct {
        uint32_t base_addr_low;
        uint32_t base_addr_high;
        uint32_t len_low;
        uint32_t len_high;
        uint32_t type;
    } data[0];
} __attribute__((packed));

static void main();

void _start()
{
    main();
}

static void *kernel_file_start;
static size_t kernel_size;

ssize_t write(int fd, const char *buf, size_t nbytes)
{
    size_t i;
    uint16_t cursor = monitor_get_cursor();
    for (i = 0; i < nbytes; i++) {
        if (buf[i] == '\n') {
            cursor += CRT_NR_COLUMNS - cursor % CRT_NR_COLUMNS;
        } else if (buf[i] == '\b') {
            cursor--;
            monitor_putchar(cursor, DEFAULT_TEXT_COLOR, '\0');
        } else {
            monitor_putchar(cursor, DEFAULT_TEXT_COLOR, buf[i]);
            cursor++;
        }

        if (cursor >= CRT_SIZE) {
            cursor -= CRT_NR_COLUMNS;
            monitor_shift(CRT_NR_COLUMNS, -CRT_NR_COLUMNS, CRT_SIZE);
        }
        monitor_set_cursor(cursor);
    }
    return 0;
}

static void error_handler(int errcode)
{
    asm("hlt");
}

static const char *memory_type(uint32_t i)
{
    switch (i) {
        case MEMORY_AVL:
            return "AVL";
        case MEMORY_RES:
            return "RES";
        default:
            return "*";
    }
}

static void print_memory_layout(void)
{
    uint16_t i;
    struct ARDS *ards = ARDS_ADDR;
    printf("\nMemory Layout:\n"
           "  %-2s %-18s %-18s %-4s\n", 
           "no", "addr", "length", "type");
    for (i = 0; i < ards->count; i++) {
        printf("  %-2d 0x%08x%08x 0x%08x%08x %s\n", i + 1,
               ards->data[i].base_addr_high,
               ards->data[i].base_addr_low,
               ards->data[i].len_high,
               ards->data[i].len_low,
               memory_type(ards->data[i].type));
    }
    printf("\n");
}

void hd_exec_cmd(uint16_t port, uint8_t data)
{
    out_byte(port, data);
    if (in_byte(ATA_PORT_CMD_STATUS) & ATA_STATUS_ERR) {
        printf("exec hd cmd error: %d\n", in_byte(ATA_PORT_ERR_NO));
        error_handler(-EERR);
    }
}

static int read_sector(uint32_t sector, uint8_t count, void *buf)
{
    int i;
    uint8_t status;
    uint16_t data;

    hd_exec_cmd(ATA_PORT_COUNT, count);
    hd_exec_cmd(ATA_PORT_SECTOR, sector & 0xff);
    hd_exec_cmd(ATA_PORT_CYLINDER_LOW, (sector >> 8) & 0xff);
    hd_exec_cmd(ATA_PORT_CYLINDER_HIGH, (sector >> 16) & 0xff);
    hd_exec_cmd(ATA_PORT_DISK_HEAD, (sector >> 24) & 0x0f | 0xe0); // 0xe0 means LBA mode and master disk
    // send request
    hd_exec_cmd(ATA_PORT_CMD_STATUS, ATA_CMD_READ);
    // wait harddisk ready
    for (i = 0; i < WAIT_HD_TIMES; i++) {
        // [BSY - - - DRQ - - ERR] DRQ: data ready
        status = in_byte(ATA_PORT_CMD_STATUS);
        if ((status & (ATA_STATUS_BUSY | ATA_STATUS_DRQ)) == ATA_STATUS_DRQ) {
            break;
        }
    }
    if (i == WAIT_HD_TIMES) {
        return -ETIMEOUT;
    }
    // read data
    for (i = 0; i < CONFIG_HD_SECT_SZ * count / sizeof(uint16_t); i++) {
        data = in_word(ATA_PORT_DATA);
        *(uint16_t *)(buf + sizeof(uint16_t) * i) = data;
    }
    return 0;
}

static int ext2_read_block(struct hd_mbr_part *part, uint32_t block, uint8_t count, void *buf)
{
    return read_sector(part->lba + EXT2_SECTS_PER_BLK * block,
                    EXT2_SECTS_PER_BLK * count, buf);
}

static inline void backspace_num(int num)
{
    int i;
    for (i = 0; i < intlen(num, 10); i++) {
        printf("\b");
    }
}

static void *kernel_file_addr(size_t filesz)
{
    int i;
    void *addr;
    struct ARDS *ards = ARDS_ADDR;
    for (i = ards->count - 1; i >= 0; i--) {
        if (ards->data[i].type == MEMORY_AVL && ards->data[i].len_low >= filesz) {
            addr = (void *)ards->data[i].base_addr_low;
            addr += ards->data[i].len_low;
            addr -= filesz;
            printf("kernel file address 0x%x\n", addr);
            return addr;
        }
    }
    printf("no space to hold kernel file!\n");
    error_handler(-EERR);
    return NULL;
}

static int read_kernel_file(void)
{
    int i, j, k, m, n, ret;
    int groups, blocks;
    bool found;
    uint8_t buf[CONFIG_EXT2_BS];
    struct hd_mbr mbr;
    struct ext2_super_block s_block;
    struct ext2_block_group group;
    struct ext2_block_group *g;
    struct hd_mbr_part *part;
    uint8_t bitmap[CONFIG_EXT2_BS];
    int inodes_per_block = CONFIG_EXT2_BS/sizeof(struct ext2_inode);
    struct ext2_inode inode_table[inodes_per_block];
    struct ext2_dir_entry kernel_ent;
    struct ext2_dir_entry *dir_ent;
    struct ext2_inode kernel_ino;
    struct ext2_inode *inode;
    void *kernel;

    ret = read_sector(0, 1, &mbr);
    if (ret != 0) {
        return ERR_READ_MBR;
    }
    for (i = 0; i < arraylen(mbr.DPT); i++) {
        if (mbr.DPT[i].boot == PART_BOOTABLE) {
            break;
        }
    }
    if (i == arraylen(mbr.DPT)) {
        return ERR_NO_BOOTABLE_PART;
    }
    part = &mbr.DPT[i];
    if (part->type != 0x83) { // EXT2
        return ERR_INVALID_FS;
    }
    ret = ext2_read_block(part, 1, 1, &s_block);
    if (ret != 0) {
        return ERR_READ_EXT2_S_BLOCK;
    }
    groups = s_block.blocks_count / s_block.blocks_per_group;
    if (s_block.blocks_count % s_block.blocks_per_group != 0) {
        groups++;
    }
    blocks = groups * sizeof(struct ext2_block_group) / CONFIG_EXT2_BS;
    if (groups * sizeof(struct ext2_block_group) % CONFIG_EXT2_BS != 0) {
        blocks++;
    }
    found = false;
    for (i = 0; i < blocks; i++) {
        ret = ext2_read_block(part, 2 + i, 1, buf);
        if (ret != 0) {
            return ERR_READ_EXT2_BLOCK_GROUP;
        }
        m = CONFIG_EXT2_BS/sizeof(struct ext2_block_group);
        for (j = 0; j < min(groups, m); j++) {
            g = (struct ext2_block_group*)buf + j;
            ret = ext2_read_block(part, g->inode_bitmap, 1, bitmap);
            if (ret != 0) {
                return ERR_READ_EXT2_INODE_MAP;
            }
            n = EXT2_INO_ROOT - s_block.first_data_block;
            if ((bitmap[n/8] >> (n%8)) & 0x1) {
                group = *g;
                found = true;
                break;
            }
        }
        if (found) {
            break;
        }
    }
    if (!found) {
        return ERR_NO_ROOT_DIR;
    }
    // search in root dir
    found = false;
    m = EXT2_INO_ROOT - s_block.first_data_block;
    ret = ext2_read_block(part, group.inode_table + m / inodes_per_block, 1, inode_table);
    if (ret != 0) {
        return ERR_READ_EXT2_INODE_TABLE;
    }
    for (i = 0; i < inode_table[m].blocks; i++) {
        ret = ext2_read_block(part, inode_table[m].block[i], 1, buf);
        if (ret != 0) {
            return ERR_READ_EXT2_ROOT_DIR;
        }
        dir_ent = (struct ext2_dir_entry *)buf;
        while ((void *)dir_ent < (void *)buf + CONFIG_EXT2_BS &&
                strcmp(dir_ent->name, KERNEL_FILE_NAME) != 0) {
            dir_ent = (void *)dir_ent + dir_ent->rec_len;
        }
        if ((void *)dir_ent == (void *)buf + CONFIG_EXT2_BS) {
            break;
        }
        kernel_ent = *dir_ent;
        found = true;
        break;
    }
    if (!found) {
        return ERR_NO_KERNEL_FILE;
    }
    m = kernel_ent.inode - s_block.first_data_block;
    ret = ext2_read_block(part, group.inode_table + m / inodes_per_block, 1, inode_table);
    if (ret != 0) {
        return ERR_READ_EXT2_INODE_TABLE;
    }
    kernel_ino = inode_table[m % inodes_per_block];
    kernel_size = kernel_ino.size;
    // read kernel file
    m = kernel_ino.size;
    if (m % CONFIG_EXT2_BS != 0) {
        m += (CONFIG_EXT2_BS - m % CONFIG_EXT2_BS);
    }
    kernel_file_start = kernel_file_addr(m);
    kernel = kernel_file_start;
    printf("reading kernel ELF...0");
    for (i = 0; i < EXT2_N_BLOCKS && kernel_ino.block[i] != 0; i++) {
        if (i < EXT2_BLOCK_L1_INDEX) {
            ret = ext2_read_block(part, kernel_ino.block[i], 1, kernel);
            if (ret != 0) {
                return ERR_READ_KERNEL_BLOCK;
            }
            backspace_num(kernel - kernel_file_start);
            kernel += CONFIG_EXT2_BS;
            printf("%d", kernel - kernel_file_start);
        } else if (i == EXT2_BLOCK_L1_INDEX) {
            ret = ext2_read_block(part, kernel_ino.block[i], 1, buf);
            if (ret != 0) {
                return ERR_READ_KERNEL_BLOCK;
            }
            for (j = 0; j < CONFIG_EXT2_BS/sizeof(uint32_t); j++) {
                m = *(uint32_t *)((void *)buf + j * sizeof(uint32_t));
                if (m == 0) {
                    break;
                }
                ret = ext2_read_block(part, m, 1, kernel);
                if (ret != 0) {
                    return ERR_READ_KERNEL_BLOCK;
                }
                backspace_num(kernel - kernel_file_start);
                kernel += CONFIG_EXT2_BS;
                printf("%d", kernel - kernel_file_start);
            }
        } else if (i == EXT2_BLOCK_L2_INDEX) {
            ret = ext2_read_block(part, kernel_ino.block[i], 1, buf);
            if (ret != 0) {
                return ERR_READ_KERNEL_BLOCK;
            }
            for (j = 0; j < CONFIG_EXT2_BS/sizeof(uint32_t); j++) {
                m = *(uint32_t *)((void *)buf + j * sizeof(uint32_t));
                if (m == 0) {
                    break;
                }
                ret = ext2_read_block(part, m, 1, bitmap); // reuse bitmap as temp variable
                if (ret != 0) {
                    return ERR_READ_KERNEL_BLOCK;
                }
                for (k = 0; k < CONFIG_EXT2_BS/sizeof(uint32_t); k++) {
                    n = *(uint32_t *)((void *)bitmap + k * sizeof(uint32_t));
                    if (n == 0) {
                        break;
                    }
                    ret = ext2_read_block(part, m, 1, kernel);
                    if (ret != 0) {
                        return ERR_READ_KERNEL_BLOCK;
                    }
                    backspace_num(kernel - kernel_file_start);
                    kernel += CONFIG_EXT2_BS;
                    printf("%d", kernel - kernel_file_start);
                }
            }
        } else if (i == EXT2_BLOCK_L3_INDEX) {
            printf("too large kernel!");
            error_handler(-EERR);
        } else {
            printf("NEVER REACH!!!");
            error_handler(-EERR);
        }
    }
    printf("\n");
    return 0;
}

static int load_kernel_elf(void **entry)
{
    int i;
    uint32_t count;
    Elf32_Ehdr *eh;
    Elf32_Phdr *ph;
    Elf32_Shdr *sh;

    eh = (Elf32_Ehdr *)kernel_file_start;
    if (strncmp(eh->e_ident, ELFMAG, SELFMAG) != 0) {
        return ERR_NON_ELF;
    }
    *entry = (void *)eh->e_entry;

    count = 0;
    for (i = 0; i < eh->e_phnum; i++) {
        ph = (Elf32_Phdr *)(kernel_file_start + eh->e_phoff + eh->e_phentsize * i);
        if (ph->p_type == PT_LOAD && ph->p_filesz > 0) {
            memcpy((void *)ph->p_paddr, kernel_file_start + ph->p_offset, ph->p_filesz);
            count++;
        }
    }
    printf("%d program(s) loaded\n", count);

    count = 0;
    for (i = 0; i < eh->e_shnum; i++) {
        sh = (Elf32_Shdr *)(kernel_file_start + eh->e_shoff + eh->e_shentsize * i);
        if (sh->sh_type == SHT_NOBITS) {
            memset((void *)sh->sh_addr, 0, sh->sh_size);
            count++;
        }
    }
    printf("%d NOBITS(s) initialized\n", count);

    // clear kernel file
    memset(kernel_file_start, 0, kernel_size);
    return 0;
}

static void *load_kernel(void)
{
    int ret;
    void *entry = NULL;

    printf("start to load kernel\n");

    ret = read_kernel_file();
    if (ret != 0) {
        printf("read kernel file error: 0x%x\n", ret);
        error_handler(ret);
    }

    ret = load_kernel_elf(&entry);
    if (ret != 0) {
        printf("load kernel ELF error: 0x%x\n", ret);
        error_handler(ret);
    }
    printf("kernel address 0x%x\n", entry);
    return entry;
}

static void main()
{
    void *entry;

    print_memory_layout();
    entry = load_kernel();
    printf("booting kernel...\n");
    asm("jmp *%0"::"p"(entry):);
}