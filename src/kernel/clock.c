#include "type.h"
#include "stdio.h"
#include "i8259a.h"
#include "sched.h"
#include "unistd.h"
#include "const.h"
#include "log.h"
#include "interrupt.h"

static int ticks = 0;

static void clock_handler() 
{
    ticks++;
    proc_sched();
    current->ticks++;
}

void init_clock() 
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