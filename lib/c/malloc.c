#include <malloc.h>

#include <kernel/mm.h>

void *malloc(size_t size)
{
    return mm_malloc(size, 0);
}

void *memalign(size_t alignment, size_t size)
{
    return mm_malloc(size, alignment);
}

void free(void *ptr)
{
    mm_free(ptr);
}