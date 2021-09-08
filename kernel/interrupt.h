#ifndef __KERNEL_INTERRUPT_H__
#define __KERNEL_INTERRUPT_H__

/* 中断向量 */
#define	IRQ_EX_DIVIDE		0x0
#define	IRQ_EX_DEBUG		0x1
#define	IRQ_EX_NMI			0x2
#define	IRQ_EX_BREAKPOINT	0x3
#define	IRQ_EX_OVERFLOW		0x4
#define	IRQ_EX_BOUNDS		0x5
#define	IRQ_EX_INVAL_OP		0x6
#define	IRQ_EX_COTASK_NOT	0x7
#define	IRQ_EX_DOUBLE_FAULT	0x8
#define	IRQ_EX_COTASK_SEG	0x9
#define	IRQ_EX_INVAL_TSS	0xa
#define	IRQ_EX_SEG_NOT		0xb
#define	IRQ_EX_STACK_FAULT	0xc
#define	IRQ_EX_PROTECTION	0xd
#define	IRQ_EX_PAGE_FAULT	0xe
#define	IRQ_EX_COTASK_ERR	0x10
#define IRQ_EX_COUNT        (IRQ_EX_COTASK_ERR - IRQ_EX_DIVIDE + 1)

typedef void (*irq_handler)(void);
typedef void (*irq_ex_handler)(int vec_no, int err_code, int eip, int cs, int eflags);

void irq_setup();
void enable_irq(int vector);
void disable_irq(int vector);
void irq_register_handler(int vector, irq_handler h);
void irq_register_ex_handler(int vector, irq_ex_handler h);

static inline void irq_enable(void)
{
    asm("sti");
}

static inline void irq_disable(void)
{
    asm("cli");
}

#endif