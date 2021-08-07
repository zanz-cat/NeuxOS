#include <stdint.h>
#include <stdio.h>

#include <drivers/io.h>
#include "floppy.h"

#define REG_DOR 0x3f2     /* Digital Output Reigster */
#define REG_STATUS 0x3f4
#define REG_DATA 0x3f5
#define REG_DIR 0x3f7 /* Digital Input Register */
#define REG_DCR 0x3f7 /* Disk Control Register */

#define DOR_DMA 0x8
#define DOR_RESET 0x4 /* 0 - reset mode, 1 - normal operation */

#define STATUS_READY 0x80 /* Data Reg ready */
#define STATUS_DIR   0x40 /* data direction, 0 - cpu to fd */

#define DATA_SPECIFY 0x03
#define DATA_RECALIBRATE 0x07
#define DATA_SEEK 0x0f
#define DATA_READ 0xe6

#define DMA_READ 0x46
#define DMA_WRITE 0x4a

#define cli() asm("cli")
#define sti() asm("sti")
#define nop() asm("nop")

static struct floppy_struct {
    uint32_t size;
    uint32_t sector;
    uint32_t head;
    uint32_t track;
    uint32_t stretch;
    uint8_t gap;
    uint8_t rate;
    uint8_t spec1;
} fd_md = {
    2880, 18, 2, 80, 0, 0x1b, 0x0, 0xcf /* 1.44MB diskette */
};

static void out_data(uint8_t data)
{
    int i;
    uint8_t status;
    for (i = 0; i < 10000; i++) {
        status = in_byte(REG_STATUS);
        if (!(status & STATUS_READY) || (status & STATUS_DIR)) {
            continue;
        }
        out_byte(REG_DATA, data);
        return;
    }
    printf("fd output data(0x%x) timeout!\n", data);
}

void func8237A(char *buf,int buff_count,char mode)
{
    out_byte(0x0D, 0x1); // this is any number to refresh 8237
    out_byte(0x0B, mode); //this is the mode of 8237 86H
    out_byte(0x08, 0); //this is also the mode contral 0h


    int temp2;
    char temp;
    temp2=(int)buf;
    temp=temp2;

    out_byte(0x04, temp); //there two are the address
    temp2=temp2>>8;
    temp=temp2;

    out_byte(0x04, temp); //first low then high


    temp2=temp2>>8;
    temp=temp2;

    out_byte(0x81, temp); //page table




    int temp1;
    temp1=buff_count;
    temp=temp1;

    out_byte(0x05, temp); //there two are the count of chars
    temp1=temp1>>8;
    temp=temp1;

    out_byte(0x05, temp); //first low then high
    out_byte(0x0F, 0xb);//remove the mask of the channel
};

static void setup_DMA(void *address, uint8_t command)
{
    cli();
    uint32_t addr = (uint32_t)address;
	if (addr >= 0x100000) {
	}
/* mask DMA 2 */
	out_byte(10, 4|2);
/* output command byte. I don't know why, but everyone (minix, */
/* sanches & canton) output this twice, first to 12 then to 11 */
 	__asm__("outb %%al,$12\n\tjmp 1f\n1:\tjmp 1f\n1:\t"
	"outb %%al,$11\n\tjmp 1f\n1:\tjmp 1f\n1:"::
	"a" ((char) ((command == DATA_READ)?DMA_READ:DMA_WRITE)));
/* 8 low bits of addr */
	out_byte(addr, 4);
/* bits 8-15 of addr */
    addr >>= 8;
	out_byte((addr),4);
/* bits 16-19 of addr */
    addr >>= 8;
	out_byte(addr,0x81);
/* low 8 bits of count-1 (1024-1=0x3ff) */
	out_byte(0xff,5);
/* high 8 bits of count-1 */
	out_byte(3,5);
/* activate DMA 2 */
	out_byte(0|2,10);
    sti();
}

void floppy_init()
{

}

int floppy_reset(uint8_t drv_no)
{
    int i;
    uint8_t dor;

    if (drv_no > FLOPPY_MAX_DRV) {
        printf("invalid floppy: %u\n", drv_no);
        return -1;
    }

    out_byte(REG_DOR, 0);
    for (i = 0; i < 100; i++) {
        asm("nop");
    }
    // select floppy and enable DMA
    dor = (1U << (4 + drv_no)) | DOR_DMA | DOR_RESET;
    out_byte(REG_DOR, dor);
    // set speed 500kbps
    out_byte(REG_DCR, 0);
    // set driver parameters
    out_data(DATA_SPECIFY);
    out_data(0xCF);
    out_data(6);

    return 0;
}

int floppy_read_sector(uint8_t drv_no, uint32_t sect_no, void *buf)
{
    uint32_t sector;
    uint32_t block;
    uint32_t track;
    uint8_t head;
    uint8_t seek_track;

    if (buf == NULL) {
        return -1;
    }

    if (sect_no >= fd_md.head * fd_md.track * fd_md.sector) {
        return -1;
    }

    sector = sect_no % fd_md.sector + 1;
    block = sect_no / fd_md.sector;
    track = block / fd_md.head;
    head = block % fd_md.head;
    seek_track = track << fd_md.stretch;

    out_data(DATA_RECALIBRATE);
    out_data(drv_no);

    out_data(DATA_SEEK);
    out_data((head << 2) | drv_no);
    out_data(seek_track);

    // func8237A(buf, sizeof(buf), 0xc6);
    setup_DMA(buf, DATA_READ);

    out_data(DATA_READ);
    out_data(drv_no);
    out_data(track);
    out_data(head);
    out_data(sector);
    out_data(2); /* 2 means 512, all floppy drives use 512bytes per sector */
    out_data(fd_md.sector);
    out_data(fd_md.gap);
    out_data(0xff);

    return 0;
}