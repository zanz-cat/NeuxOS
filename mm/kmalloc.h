#ifndef __KERNEL_KMALLOC_H__
#define __KERNEL_KMALLOC_H__

#include <stddef.h>

void *kmalloc(size_t size);
void kfree(void *ptr);
void *kmemalign(size_t alignment, size_t size);
void init_heap(void);
void heap_report(void);

#endif