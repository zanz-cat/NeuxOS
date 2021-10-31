#ifndef __I8259A_H__
#define __I8259A_H__

/* 中断向量 */
#define	INT_VECTOR_IRQ0 0x20
#define	INT_VECTOR_IRQ8 0x28

/* 8259A interrupt controller ports. */
#define INT_M_CTL     0x20 /* I/O port for interrupt controller       <Master> */
#define INT_M_CTLMASK 0x21 /* setting bits in this port disables ints <Master> */
#define INT_S_CTL     0xa0 /* I/O port for second interrupt controller<Slave>  */
#define INT_S_CTLMASK 0xa1 /* setting bits in this port disables ints <Slave>  */

#define IRQ_CLOCK           (INT_VECTOR_IRQ0 + 0x0)
#define IRQ_KEYBOARD        (INT_VECTOR_IRQ0 + 0x1)
#define IRQ_SLAVE           (INT_VECTOR_IRQ0 + 0x2)
#define IRQ_SERIAL2         (INT_VECTOR_IRQ0 + 0x3)
#define IRQ_SERIAL1         (INT_VECTOR_IRQ0 + 0x4)
#define IRQ_LPT2            (INT_VECTOR_IRQ0 + 0x5)
#define IRQ_FLOPPY          (INT_VECTOR_IRQ0 + 0x6)
#define IRQ_LPT1            (INT_VECTOR_IRQ0 + 0x7)
#define IRQ_REAL_CLOCK      (INT_VECTOR_IRQ0 + 0x8)
#define IRQ_MOUSE           (INT_VECTOR_IRQ0 + 0xc)
#define IRQ_COPR            (INT_VECTOR_IRQ0 + 0xd)
#define IRQ_HARDDISK        (INT_VECTOR_IRQ0 + 0xe)
#define IRQ_RESV            (INT_VECTOR_IRQ0 + 0xf)
#define IRQ_COUNT           (IRQ_HARDDISK - IRQ_CLOCK + 1)

void setup_8259a(void);
void enable_irq_n(int vector);
void disable_irq_n(int vector);

#endif