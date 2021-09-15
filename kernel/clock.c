#include <stdint.h>

#include <drivers/io.h>
#include <drivers/i8259a.h>

#include "log.h"
#include "sched.h"
#include "interrupt.h"
#include "printk.h"

#include "clock.h"

/* 8253/8254 PIT */
#define PIT_TIMER0 0x40
#define PIT_TIMER_MODE 0x43
#define PIT_TIMER_RATE_GEN 0x34
#define PIT_FREQ 1193182L
#define PIT_HZ 1000

static uint64_t jeffies;

uint64_t rdtsc(void)
{
    uint32_t a, d;
    asm("rdtsc":"=a"(a), "=d"(d)::);
    return ((uint64_t)d << 32) | a;
}

static void clock_handler()
{
    jeffies++;
    sched_task();
}

void clock_setup()
{
    log_info("setup clock\n");

    irq_register_handler(IRQ_CLOCK, clock_handler);

    /* 设置时钟频率 */
    out_byte(PIT_TIMER_MODE, PIT_TIMER_RATE_GEN);
    out_byte(PIT_TIMER0, (uint8_t)(PIT_FREQ/PIT_HZ));
    out_byte(PIT_TIMER0, (uint8_t)((PIT_FREQ/PIT_HZ) >> 8));

    /* 开启时钟中断 */
    enable_irq_n(IRQ_CLOCK);
}

uint32_t kget_jeffies()
{
    return jeffies;
}