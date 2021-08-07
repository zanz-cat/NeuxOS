#ifndef __I8259A_H__
#define __I8259A_H__

#define IRQ_CLOCK           0x0
#define IRQ_KEYBOARD        0x1
#define IRQ_SERIAL2         0x3
#define IRQ_SERIAL1         0x4
#define IRQ_LPT2            0x5
#define IRQ_FLOPPY          0x6
#define IRQ_LPT1            0x7
#define IRQ_REAL_CLOCK      0x8
#define IRQ_MOUSE           0xc
#define IRQ_COPR            0xd
#define IRQ_HARDDISK        0xe

void init_8259a();
void enable_irq(int vector);
void disable_irq(int vector);

#endif