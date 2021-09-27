#include <kernel/mm.h>

#include "kmalloc.h"

void *kmalloc(size_t size)
{
    return mm_malloc(size, 0);
}

void *kmemalign(size_t alignment, size_t size)
{
    return mm_malloc(size, alignment);
}

void kfree(void *ptr)
{
    mm_free(ptr);
}