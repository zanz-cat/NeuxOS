#ifndef __DRIVER_HARDDISK_H__
#define __DRIVER_HARDDISK_H__

#include <stddef.h>
#include <stdint.h>

#define ATA_PORT_DATA 0x01f0
#define ATA_PORT_FEATURES 0x01f1
#define ATA_PORT_ERR_NO ATA_PORT_FEATURES
#define ATA_PORT_COUNT 0x01f2
#define ATA_PORT_SECTOR 0x01f3 // or LBA(7~0)
#define ATA_PORT_CYLINDER_LOW 0x01f4 // or LBA(15~8)
#define ATA_PORT_CYLINDER_HIGH 0x01f5 // or LBA(23~16)
#define ATA_PORT_DISK_HEAD 0x01f6 // '1 x 1 y z z z z' x=1 LBA mode, y=1 slave, z is HEAD or LBA(27 ~ 24)
#define ATA_PORT_CMD 0x01f7
#define ATA_PORT_STATUS ATA_PORT_CMD
#define ATA_PORT_DEV_CTRL 0x03f6

#define ATA_CMD_READ 0x20

#define ATA_STATUS_BUSY 0x80
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_ERR 0x01

struct hd_mbr_part {
    uint8_t boot; // 0x80 means active
    uint8_t s_head;
    uint16_t s_sector_cylinder;  // bits: 0 ~ 5 for sector, 6 ~ 7 for cylinder high 2 bits, 8 ~ 15 cylinder low 16 bits
    uint8_t type;
    uint8_t e_head;
    uint16_t e_sector_cylinder;
    uint32_t lba;
    uint32_t sectors;
} __attribute__((packed));

#define PART_BOOTABLE 0x80
#define PART_TYPE_EXT2FS 0x83

#define HD_MBR_DPT_SIZE 4
#define HD_MBR_END_MAGIC 0xaa55

#define HD_MBR_PART_SECTOR(sector_cylinder) ((uint8_t)(sector_cylinder) & 0x3f)
#define HD_MBR_PART_CYLINDER(sector_cylinder) \
        (((uint8_t)(sector_cylinder) & 0xc0 << 8) | ((sector_cylinder) >> 8))

struct hd_mbr {
    uint8_t code[446];
    struct hd_mbr_part DPT[HD_MBR_DPT_SIZE]; // Dos Partition Table
    uint8_t end_magic[2];
} __attribute__((packed));

void hd_setup(void);
int hd_read(uint32_t sector, uint8_t count, void *buf, size_t size);

#endif