#include <stdbool.h>
#include <errno.h>

#include <config.h>
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

static bool is_kernel_stack(void *ebp)
{
    return ebp >= (void *)CONFIG_KERNEL_VM_OFFSET && ebp < (void *)current->tss.esp0;
}

static bool is_user_stack(void *ebp)
{
    return ebp < (void *)CONFIG_KERNEL_VM_OFFSET;
}

int backtrace(void **buf, int size)
{
    int i;
    void **ebp;
    if (size < 0) {
        return -EINVAL;
    }
    asm("movl %%ebp, %0":"=r"(ebp)::);

    // kernel stack
    for (i = 0; i < size && is_kernel_stack(*ebp); i++, ebp = *ebp) {
        buf[i] = *(ebp + 1);
    }

    // user stack
    for (; i < size && is_user_stack(*ebp); i++, ebp = *ebp) {
        buf[i] = *(ebp + 1);
    }
    return i;
}