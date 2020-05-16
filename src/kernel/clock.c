#include "stdio.h"
#include "schedule.h"

void clock_int_handler() {
    next_proc();
}