#include "type.h"
#include "stdio.h"
#include "i8259a.h"
#include "schedule.h"

#define CLOCK_INT_STACK_SIZE 1024

u8  clock_int_stack[CLOCK_INT_STACK_SIZE];
void *clock_int_stacktop;

void clock_int_handler() {
    next_proc();
    send_eoi();
}