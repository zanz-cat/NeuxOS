#include "type.h"
#include "stdio.h"
#include "i8259a.h"
#include "schedule.h"
#include "unistd.h"

static int ticks = 0;

void clock_handler() {
    ticks++;
    proc_sched();
}

int sys_get_ticks() {
    return ticks;
}