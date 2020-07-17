#include "type.h"
#include "stdio.h"
#include "i8259a.h"
#include "sched.h"
#include "unistd.h"

int ticks = 0;

void clock_handler() {
    ticks++;
    proc_sched();
}
