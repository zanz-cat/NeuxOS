#include <kernel/interrupt.h>
#include <kernel/protect.h>

#include "i8259a.h"

void init_8259a()
{
    out_byte(INT_M_CTL, 0x11);
    out_byte(INT_S_CTL, 0x11);

    out_byte(INT_M_CTLMASK, INT_VECTOR_IRQ0);
    out_byte(INT_S_CTLMASK, INT_VECTOR_IRQ8);

    out_byte(INT_M_CTLMASK, 0x4);
    out_byte(INT_S_CTLMASK, 0x2);

    out_byte(INT_M_CTLMASK, 0x1);
    out_byte(INT_S_CTLMASK, 0x1);

    out_byte(INT_M_CTLMASK, 0xff);
    out_byte(INT_S_CTLMASK, 0xff);
}

void enable_irq(int vector)
{
    int port = vector < 8 ? INT_M_CTLMASK : INT_S_CTLMASK;
    uint8_t mask = in_byte(port);
    out_byte(port, mask & (~(0x1 << vector)));
}

void disable_irq(int vector)
{
    int port = vector < 8 ? INT_M_CTLMASK : INT_S_CTLMASK;
    uint8_t mask = in_byte(port);
    out_byte(port, mask & (0x1 << vector));
}