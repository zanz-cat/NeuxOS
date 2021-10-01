#ifndef __KERNEL_MEMORY_H__
#define __KERNEL_MEMORY_H__

#include <stddef.h>
#include <config.h>

#define virt_addr(addr) ((typeof(addr))((uint32_t)(addr) + CONFIG_KERNEL_VM_OFFSET))
#define phy_addr(addr) ((typeof(addr))((uint32_t)(addr) - CONFIG_KERNEL_VM_OFFSET))

size_t get_total_memory(void);

#endif