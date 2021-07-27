#include <type.h>
#include <lib/log.h>
#include <kernel/i8259a.h>
#include <kernel/sched.h>
#include <kernel/const.h>
#include <kernel/interrupt.h>

static int ticks = 0;

static void clock_handler() 
{
    ticks++;
    proc_sched();
    current->ticks++;
}

void clock_init() 
{
    log_info("init clock\n");

    put_irq_handler(IRQ_CLOCK, clock_handler);

    /* 设置时钟频率 */
    out_byte(TIMER_MODE, RATE_GENERATOR);
    out_byte(TIMER0, (u8)(TIMER_FREQ/HZ));
    out_byte(TIMER0, (u8)((TIMER_FREQ/HZ) >> 8));

    /* 开启时钟中断 */
    enable_irq(IRQ_CLOCK);
}

int kget_ticks() 
{
    return ticks;
}