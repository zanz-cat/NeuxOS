#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

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

typedef void (*int_handler)();

void init_interrupt();
void put_irq_handler(int vector, int_handler h);
void enable_irq(int vector);
void disable_irq(int vector);

// defined in interrupt.S
void divide_error();
void single_step_exception();
void nmi();
void breakpoint_exception();
void overflow();
void bounds_check();
void inval_opcode();
void copr_not_available();
void double_fault();
void copr_seg_overrun();
void inval_tss();
void segment_not_present();
void stack_exception();
void general_protection();
void page_fault();
void copr_error();
void syscall();

void hwint00();
void hwint01();
void hwint03();
void hwint04();
void hwint05();
void hwint06();
void hwint07();
void hwint08();
void hwint12();
void hwint13();
void hwint14();

#endif