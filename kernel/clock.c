#include <stdint.h>

#include <lib/log.h>
#include <drivers/i8259a/i8259a.h>

#include "sched.h"
#include "kernel.h"
#include "interrupt.h"

static uint32_t jeffies = 0;

static void clock_handler()
{
    jeffies++;
    proc_sched();
    current->ticks++;
}

void clock_init()
{
    log_info("init clock\n");

    put_irq_handler(IRQ_CLOCK, clock_handler);

    /* 设置时钟频率 */
    out_byte(TIMER_MODE, RATE_GENERATOR);
    out_byte(TIMER0, (uint8_t)(TIMER_FREQ/HZ));
    out_byte(TIMER0, (uint8_t)((TIMER_FREQ/HZ) >> 8));

    /* 开启时钟中断 */
    enable_irq(IRQ_CLOCK);
}

uint32_t kget_jeffies()
{
    return jeffies;
}