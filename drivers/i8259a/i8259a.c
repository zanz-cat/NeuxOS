#include <arch/x86.h>
#include <kernel/interrupt.h>

#include "drivers/io.h"
#include "i8259a.h"

void setup_8259a(void)
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

void enable_irq_n(int vector)
{
    int port = (vector - INT_VECTOR_IRQ0) < 8 ? INT_M_CTLMASK : INT_S_CTLMASK;
    int pvec = vector - (port == INT_M_CTLMASK ? INT_VECTOR_IRQ0 : INT_VECTOR_IRQ8);
    uint8_t mask = in_byte(port);
    out_byte(port, mask & (~(0x1 << pvec)));
}

void disable_irq_n(int vector)
{
    int port = (vector - INT_VECTOR_IRQ0) < 8 ? INT_M_CTLMASK : INT_S_CTLMASK;
    int pvec = vector - (port == INT_M_CTLMASK ? INT_VECTOR_IRQ0 : INT_VECTOR_IRQ8);
    uint8_t mask = in_byte(port);
    out_byte(port, mask & (0x1 << pvec));
}