#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <elf.h>
#include <errno.h>

#include <drivers/io.h>
#include <drivers/harddisk.h>
#include <drivers/monitor.h>
#include <mm/mm.h>
#include <kernel/memory.h>
#include <misc/misc.h>
#include <fs/ext2.h>

#define HD_TIMEOUT 100000
#define KERNEL_FILE_NAME "kernel.elf"

#define ERR_READ_MBR 100
#define ERR_NO_BOOTABLE_PART 101
#define ERR_INVALID_FS 102
#define ERR_READ_EXT2_S_BLOCK 103
#define ERR_READ_EXT2_BLOCK_GROUP 104
#define ERR_READ_EXT2_INODE_MAP 105
#define ERR_NO_ROOT_DIR 106
#define ERR_READ_EXT2_INODE_TABLE 107
#define ERR_READ_EXT2_ROOT_DIR 108
#define ERR_NO_KERNEL_FILE 109
#define ERR_READ_KERNEL_BLOCK 110

static void load();

void _start()
{
    load();
}

static void *kernel_file;
static size_t kernel_size;

ssize_t write(int fd, const char *buf, size_t nbytes)
{
    static struct monitor _mon;
    static struct monitor *mon = NULL;
    if (mon == NULL) {
        monitor_init(&_mon, 0);
        monitor_switch(NULL, &_mon);
        mon = &_mon;
    }

    size_t i;
    for (i = 0; i < nbytes; i++) {
        monitor_putchar(mon, buf[i]);
    }
    return i;
}

static void loader_panic(int errcode)
{
    asm("hlt");
}

static const char *memory_type(uint32_t i)
{
    switch (i) {
        case MEM_TYPE_AVL:
            return "AVL";
        case MEM_TYPE_RES:
            return "RES";
        default:
            return "*";
    }
}

static void print_memory_layout(void)
{
    uint16_t i;

    printf("Memory Layout:\n"
           "%-2s %-18s %-18s %-4s\n", 
           "no", "addr", "length", "type");
    for (i = 0; i < SHARE_DATA()->ards_cnt; i++) {
        printf("%-2d 0x%08x%08x 0x%08x%08x %s\n", i + 1,
               SHARE_DATA()->ards[i].base_addr_high,
               SHARE_DATA()->ards[i].base_addr_low,
               SHARE_DATA()->ards[i].len_high,
               SHARE_DATA()->ards[i].len_low,
               memory_type(SHARE_DATA()->ards[i].type));
    }
}

static int wait(void)
{
    uint32_t i;
    uint8_t status;
    
    for (i = 0; i < HD_TIMEOUT; i++) {
        // [BSY - - - DRQ - - ERR] DRQ: data ready
        status = in_byte(ATA_PORT_STATUS);
        if ((status & (ATA_STATUS_BUSY | ATA_STATUS_DRQ)) == ATA_STATUS_DRQ) {
            return 0;
        }
    }
    return -ETIMEDOUT;
}

static size_t read_one_sector(void *buf)
{
    uint32_t i;
    size_t n = 0;

    for (i = 0; i < CONFIG_HD_SECT_SZ/sizeof(uint16_t); i++) {
        *(uint16_t *)(buf + n) = in_word(ATA_PORT_DATA);
        n += sizeof(uint16_t);
    }
    return n;
}

static int loader_read_sector(uint32_t sector, uint8_t count, void *buf)
{
    int i;
    uint8_t status;
    size_t offset = 0;

    for (i = 0; i < HD_TIMEOUT; i++) {
        status = in_byte(ATA_PORT_STATUS);
        if (!(status & ATA_STATUS_BUSY)) {
            break;
        }
    }
    if (i == HD_TIMEOUT) {
        return -ETIMEDOUT;
    }
    out_byte(ATA_PORT_COUNT, count);
    out_byte(ATA_PORT_SECTOR, sector & 0xff);
    out_byte(ATA_PORT_CYLINDER_LOW, (sector >> 8) & 0xff);
    out_byte(ATA_PORT_CYLINDER_HIGH, (sector >> 16) & 0xff);
    out_byte(ATA_PORT_DISK_HEAD, ((sector >> 24) & 0x0f) | 0xe0); // 0xe0 means LBA mode and master disk
    // send request
    out_byte(ATA_PORT_CMD, ATA_CMD_READ);
    for (i = 0; i < count; i++) {
        if (wait() != 0) {
            return -ETIMEDOUT;
        }
        offset += read_one_sector(buf + offset);
    }
    return 0;
}

static int ext2_read_block(struct hd_mbr_part *part, uint32_t block, uint8_t count, void *buf)
{
    int ret = loader_read_sector(part->lba + EXT2_SECTS_PER_BLK * block,
                                 EXT2_SECTS_PER_BLK * count, buf);
    if (ret != 0) {
        printf("loader_read_sector error: %d\n", ret);
    }
    return ret;
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

    for (i = SHARE_DATA()->ards_cnt - 1; i >= 0; i--) {
        if (SHARE_DATA()->ards[i].type == MEM_TYPE_AVL && SHARE_DATA()->ards[i].len_low >= filesz) {
            addr = (void *)SHARE_DATA()->ards[i].base_addr_low;
            addr += SHARE_DATA()->ards[i].len_low;
            addr -= filesz;
            printf("kernel file address 0x%x\n", addr);
            return addr;
        }
    }
    printf("no space to hold kernel file!\n");
    loader_panic(-ENOMEM);
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
    void *kernel;

    ret = loader_read_sector(0, 1, &mbr);
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
    if (part->type != PART_TYPE_EXT2FS) {
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
    kernel_file = kernel_file_addr(m);
    kernel = kernel_file;
    printf("reading kernel ELF...0");
    for (i = 0; i < EXT2_N_BLOCKS && kernel_ino.block[i] != 0; i++) {
        if (i < EXT2_B_L1_IDX) {
            ret = ext2_read_block(part, kernel_ino.block[i], 1, kernel);
            if (ret != 0) {
                printf("\n");
                return ERR_READ_KERNEL_BLOCK;
            }
            backspace_num(kernel - kernel_file);
            kernel += CONFIG_EXT2_BS;
            printf("%d", kernel - kernel_file);
        } else if (i == EXT2_B_L1_IDX) {
            ret = ext2_read_block(part, kernel_ino.block[i], 1, buf);
            if (ret != 0) {
                printf("\n");
                return ERR_READ_KERNEL_BLOCK;
            }
            for (j = 0; j < CONFIG_EXT2_BS/sizeof(uint32_t); j++) {
                m = *(uint32_t *)((void *)buf + j * sizeof(uint32_t));
                if (m == 0) {
                    break;
                }
                ret = ext2_read_block(part, m, 1, kernel);
                if (ret != 0) {
                    printf("\n");
                    return ERR_READ_KERNEL_BLOCK;
                }
                backspace_num(kernel - kernel_file);
                kernel += CONFIG_EXT2_BS;
                printf("%d", kernel - kernel_file);
            }
        } else if (i == EXT2_B_L2_IDX) {
            ret = ext2_read_block(part, kernel_ino.block[i], 1, buf);
            if (ret != 0) {
                printf("\n");
                return ERR_READ_KERNEL_BLOCK;
            }
            for (j = 0; j < CONFIG_EXT2_BS/sizeof(uint32_t); j++) {
                m = *(uint32_t *)((void *)buf + j * sizeof(uint32_t));
                if (m == 0) {
                    break;
                }
                ret = ext2_read_block(part, m, 1, bitmap); // reuse bitmap as temp variable
                if (ret != 0) {
                    printf("\n");
                    return ERR_READ_KERNEL_BLOCK;
                }
                for (k = 0; k < CONFIG_EXT2_BS/sizeof(uint32_t); k++) {
                    n = *(uint32_t *)((void *)bitmap + k * sizeof(uint32_t));
                    if (n == 0) {
                        break;
                    }
                    ret = ext2_read_block(part, m, 1, kernel);
                    if (ret != 0) {
                        printf("\n");
                        return ERR_READ_KERNEL_BLOCK;
                    }
                    backspace_num(kernel - kernel_file);
                    kernel += CONFIG_EXT2_BS;
                    printf("%d", kernel - kernel_file);
                }
            }
        } else if (i == EXT2_B_L3_IDX) {
            printf("\ntoo large kernel!");
            loader_panic(-ENOMEM);
        } else {
            printf("\nNEVER REACH!!!");
            loader_panic(-1);
        }
    }
    printf("\n");
    return 0;
}

static int load_kernel_elf(uint32_t *entry)
{
    int i;
    uint32_t count;
    Elf32_Ehdr *eh;
    Elf32_Phdr *ph;
    Elf32_Shdr *sh;

    SHARE_DATA()->kernel_start = 0xffffffff;
    SHARE_DATA()->kernel_end = 0;
    eh = (Elf32_Ehdr *)kernel_file;
    if (strncmp((char *)eh->e_ident, ELFMAG, SELFMAG) != 0) {
        return -EINVAL;
    }
    *entry = eh->e_entry;

    count = 0;
    for (i = 0; i < eh->e_phnum; i++) {
        ph = (Elf32_Phdr *)(kernel_file + eh->e_phoff + eh->e_phentsize * i);
        if (ph->p_type == PT_LOAD && ph->p_filesz > 0 && ph->p_vaddr >= CONFIG_KERNEL_VM_OFFSET) {
            memcpy((void *)phy_addr(ph->p_vaddr), kernel_file + ph->p_offset, ph->p_filesz);
            if (SHARE_DATA()->kernel_start > phy_addr(ph->p_vaddr)) {
                SHARE_DATA()->kernel_start = phy_addr(ph->p_vaddr);
            }
            if (phy_addr(ph->p_vaddr) + ph->p_memsz > SHARE_DATA()->kernel_end) {
                SHARE_DATA()->kernel_end = phy_addr(ph->p_vaddr) + ph->p_memsz;
            }
            count++;
        }
    }
    printf("%d program(s) loaded\n", count);

    count = 0;
    for (i = 0; i < eh->e_shnum; i++) {
        sh = (Elf32_Shdr *)(kernel_file + eh->e_shoff + eh->e_shentsize * i);
        if (sh->sh_type == SHT_NOBITS && sh->sh_addr >= CONFIG_KERNEL_VM_OFFSET) {
            memset((void *)phy_addr(sh->sh_addr), 0, sh->sh_size);
            if (SHARE_DATA()->kernel_start > phy_addr(sh->sh_addr)) {
                SHARE_DATA()->kernel_start = phy_addr(sh->sh_addr);
            }
            if (phy_addr(sh->sh_addr) + sh->sh_size > SHARE_DATA()->kernel_end) {
                SHARE_DATA()->kernel_end = phy_addr(sh->sh_addr) + sh->sh_size;
            }
            count++;
        }
    }
    printf("%d BSS(s) initialized\n", count); // Block Started by Symbol

    // clear kernel file
    memset(kernel_file, 0, kernel_size);

    printf("kernel loaded in 0x%x ~ 0x%x\n", SHARE_DATA()->kernel_start, SHARE_DATA()->kernel_end);

    return 0;
}

static uint32_t load_kernel(void)
{
    int ret;
    uint32_t entry = 0;

    printf("start to load kernel\n");

    ret = read_kernel_file();
    if (ret != 0) {
        printf("read kernel file error: %d\n", ret);
        loader_panic(ret);
    }

    ret = load_kernel_elf(&entry);
    if (ret != 0) {
        printf("load kernel ELF error: %d\n", ret);
        loader_panic(ret);
    }
    return entry;
}

static void enable_paging(void)
{
    asm("movl %0, %%eax\n\t"
        "movl %%eax, %%cr3\n\t"
        "movl %%cr0, %%eax\n\t"
        "or %1, %%eax\n\t"
        "movl %%eax, %%cr0"::"i"(CONFIG_KERNEL_PG_ADDR),"i"(0x80000000):"%eax");
}

static void setup_page(void)
{
    int i, j;
    uint32_t page = 0;
    struct page_entry *pgdir = (void *)CONFIG_KERNEL_PG_ADDR;
    struct page_entry *pgtbl = pgdir + PAGE_ENTRY_COUNT;

    memset(pgdir, 0, PAGE_SIZE);
    for (i = (CONFIG_KERNEL_VM_OFFSET >> PGDIR_SHIFT); i < PAGE_ENTRY_COUNT; i++) {
        pgdir[i].present = 1;
        pgdir[i].rw = 1;
        pgdir[i].index = (uint32_t)pgtbl >> PAGE_SHIFT;
        memset(pgtbl, 0, PAGE_SIZE);
        for (j = 0; j < PAGE_ENTRY_COUNT; j++) {
            pgtbl[j].present = 1;
            pgtbl[j].rw = 1;
            pgtbl[j].index = page++;
        }
        pgtbl += PAGE_ENTRY_COUNT;
    }
    /* create temporary page table for 0~1MB memory
     * only valid in loader, kernel should uninstall 
     * this page table.
     */
    pgtbl = (void *)ALIGN_CEIL(SHARE_DATA()->kernel_end, PAGE_SIZE);
    pgdir[0].present = 1;
    pgdir[0].rw = 0;
    pgdir[0].index = (uint32_t)pgtbl / PAGE_SIZE;
    memset(pgtbl, 0, PAGE_SIZE);
    for (j = 0; j < (0x100000 >> PAGE_SHIFT); j++) {
        pgtbl[j].present = 1;
        pgtbl[j].rw = 0;
        pgtbl[j].index = j;
    }
    enable_paging();
}

static void load()
{
    uint32_t entry_point[2] = { 0 };
    printf("loading kernel...\n");
    print_memory_layout();
    entry_point[0] = load_kernel();
    entry_point[1] = SELECTOR_KERNEL_CS;
    setup_page();
    asm("jmp *(%0)"::"p"(entry_point):);
}