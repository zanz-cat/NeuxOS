#include <stdio.h>
#include <assert.h>

#include "arch/x86.h"

#include "config.h"

/*
# compute kernel image vm address
#
# physical memory layout
# + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# | basic RAM | page table  |  kernel image | reserved |
# + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# 0           1M            2M+4K
#
# reserved area for temporary using and avoiding overflow override kernel code.
# reserved area is aligned with page size
# basicRAM page table is in reserved area
# 
*/

int main(int argc, char *argv[])
{

    printf("CONFIG_KERNEL_IMG_VM_ADDR=0x%x\n", CONFIG_KERNEL_VM_OFFSET + (1<<20) + (1<<12) + (1<<20));
    printf("CONFIG_KERNEL_PG_ADDR=0x100000\n");
    return 0;
}