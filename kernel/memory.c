#include <stddef.h>

#include <arch/x86.h>
#include <lib/misc.h>

#include "memory.h"

static size_t total_mem = 0;

size_t get_total_memory(void)
{
    uint32_t i;

    if (total_mem != 0) {
        return total_mem;
    }

    for (i = SHARE_DATA()->ards_cnt - 1; i >= 0; i--) {
        if (SHARE_DATA()->ards[i].type == MEM_TYPE_AVL) {
            total_mem = SHARE_DATA()->ards[i].base_addr_low + SHARE_DATA()->ards[i].len_low;
            break;
        }
    }
    return total_mem;
}