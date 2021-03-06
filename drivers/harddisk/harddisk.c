#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>

#include <config.h>
#include <kernel/kernel.h>
#include <kernel/interrupt.h>
#include <kernel/sched.h>
#include <kernel/lock.h>
#include <kernel/log.h>
#include <mm/kmalloc.h>
#include <drivers/io.h>
#include <lib/list.h>
#include <lib/misc.h>

#include "harddisk.h"

#define HD_TIMEOUT 10000

static LIST_HEAD(wait_queue);
static struct simplock lock = SIMPLOCK_INITIALIZER;

static void hd_irq_handler(void)
{
    uint8_t status;

    status = in_byte(ATA_PORT_STATUS);
    if ((status & (ATA_STATUS_BUSY | ATA_STATUS_DRQ)) != ATA_STATUS_DRQ) {
        return;
    }
    if (LIST_COUNT(&wait_queue) == 0) {
        return;
    }
    task_resume(&wait_queue);
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

static size_t read_one_sector(void *buf, size_t size)
{
    uint32_t i;
    uint16_t data;
    size_t n = 0;

    for (i = 0; i < CONFIG_HD_SECT_SZ/sizeof(uint16_t); i++) {
        if (n == size - 1) {
            data = in_word(ATA_PORT_DATA);
            *(uint8_t *)(buf + n) = (uint8_t)data;
            n++;
        } else if (n < size) {
            *(uint16_t *)(buf + n) = in_word(ATA_PORT_DATA);
            n += sizeof(uint16_t);
        } else {
            // drop
            in_word(ATA_PORT_DATA);
        }
    }
    return n;
}

static int do_read_sync(uint8_t count, void *buf, size_t size)
{
    uint32_t i;
    size_t offset = 0;

    // send request
    out_byte(ATA_PORT_CMD, ATA_CMD_READ);
    if (wait() != 0) {
        return -ETIMEDOUT;
    }
    for (i = 0; i < count && offset < size; i++) {
        offset += read_one_sector(buf + offset, size - offset);
    }
    return offset;
}

static int do_read_async(uint8_t count, void *buf, size_t size)
{
    uint32_t i;
    size_t offset = 0;

    disable_irq();
    // send request
    out_byte(ATA_PORT_CMD, ATA_CMD_READ);
    for (i = 0; i < count; i++) {
        task_suspend(&wait_queue);
        offset += read_one_sector(buf + offset, size - offset);
    }
    enable_irq();
    return offset;
}

int hd_read(uint32_t sector, uint8_t count, void *buf, size_t size)
{
    int res;
    uint32_t i;
    uint8_t status;

    if (count == 0) {
        return -EINVAL;
    }

    simplock_obtain(&lock);
    for (i = 0; i < HD_TIMEOUT; i++) {
        status = in_byte(ATA_PORT_STATUS);
        if (!(status & ATA_STATUS_BUSY)) {
            break;
        }
    }
    if (i == HD_TIMEOUT) {
        log_error("hd busy\n");
        res = -ETIMEDOUT;
        goto out;
    }
    out_byte(ATA_PORT_DEV_CTRL, 0); // enable interrupt
    out_byte(ATA_PORT_FEATURES, 0);
    out_byte(ATA_PORT_COUNT, count);
    out_byte(ATA_PORT_SECTOR, sector & 0xff);
    out_byte(ATA_PORT_CYLINDER_LOW, (sector >> 8) & 0xff);
    out_byte(ATA_PORT_CYLINDER_HIGH, (sector >> 16) & 0xff);
    out_byte(ATA_PORT_DISK_HEAD, ((sector >> 24) & 0x0f) | 0xe0); // 0xe0 means LBA mode and master disk
    res = (eflags() & EFLAGS_IF) ? do_read_async(count, buf, size) : do_read_sync(count, buf, size);

out:
    simplock_release(&lock);
    return res;
}

void hd_setup(void)
{
    log_info("setup harddisk\n");
    irq_register_handler(IRQ_HARDDISK, hd_irq_handler);
    enable_irq_n(IRQ_HARDDISK);
}