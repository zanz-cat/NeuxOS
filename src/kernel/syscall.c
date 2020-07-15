typedef void* syscall_handler;

int sys_get_ticks();

syscall_handler syscall_handler_table[] = {
    sys_get_ticks
};