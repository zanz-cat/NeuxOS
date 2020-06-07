#include "type.h"
#include "stdio.h"
#include "i8259a.h"
#include "schedule.h"
#include "unistd.h"

void clock_handler() {
    next_proc();
}