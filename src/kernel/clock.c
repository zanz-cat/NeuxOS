#include "stdio.h"
#include "schedule.h"

t_proc *clock_int_handler() {
    return next_proc();
}