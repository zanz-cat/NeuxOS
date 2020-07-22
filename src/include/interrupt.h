#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

typedef void (*int_handler)();

void init_interrupt();
void put_irq_handler(int vector, int_handler h);
void enable_irq(int vector);
void disable_irq(int vector);

#define IRQ_CLOCK           0x0
#define IRQ_KEYBOARD        0x1

#endif