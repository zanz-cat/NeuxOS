#include "clock.h"

typedef void* syscall_handler;

int sys_get_ticks() 
{
    return kget_ticks();
}

syscall_handler syscall_handler_table[] = {
    sys_get_ticks
};