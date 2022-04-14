#include <errno.h>
#include <kernel/sched.h>

#include "arch/backtrace.h"

/**
 * @brief get call stack
 * 
 * @param buf 
 * @param size 
 * @return int count of stack, negative means error occured
 * 
 * top
 * |            |  ebp
 * |            |  /
 * |            | /
 * |    ebp2    | --+
 * +------------+   |
 * |    ip2     |   |
 * |            |   |
 * |            |   |
 * |    ebp1    | <-+
 * +------------+  \
 * |    ip1     |   |
 * |            |   |
 * |            |   |
 * |            |   |
 * |    ebp0    | <-+
 * +------------+
 * |    ip0     |
 * |            |
 * |            |
 * +------------+
 * 
 */
int backtrace(void **buf, int size)
{
    int i;
    void **ebp;
    if (size < 0) {
        return -EINVAL;
    }
    asm("movl %%ebp, %0":"=r"(ebp)::);

    for (i = 0; i < size && *ebp < current->stack0; i++, ebp = *ebp) {
        buf[i] = *(ebp + 1);
    }
    return i;
}