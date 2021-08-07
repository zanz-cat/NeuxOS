#ifndef __DRIVER_FLOPPY_H__
#define __DRIVER_FLOPPY_H__

#define FLOPPY_MAX_DRV 3

void floppy_init();
int floppy_reset(uint8_t drv_no);
int floppy_read_sector(uint8_t drv_no, uint32_t sect_no, void *buf);

#endif