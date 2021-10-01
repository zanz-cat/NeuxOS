#include <stdio.h>

#include "arch/x86.h"

#include "config.h"

#define CHECK(name, cond, desc) \
do { \
    printf("checking %s...", name); \
    fflush(stdout); \
    if (!(cond)) { \
        printf("%s\n", desc); \
        return -1; \
    } \
    printf("OK\n"); \
} while(0);

int main(int argc, char *argv[])
{
    CHECK("CONFIG_KERNEL_VM_OFFSET", 
          ((CONFIG_KERNEL_VM_OFFSET & ((1U << PGDIR_SHIFT) - 1)) == 0),
          "must aligned with 4MB");

    return 0;
}