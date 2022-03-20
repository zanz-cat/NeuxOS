#ifndef __LIB_BITMAP_H__
#define __LIB_BITMAP_H__

#include <stdint.h>
#include <stddef.h>

struct bitmap {
    size_t size; // in bit
    uint8_t bits[0];
};

// get memory size
size_t bitmap_msize(size_t bitsize);
void bitmap_init(struct bitmap *bp, size_t bitsize);
int bitmap_get(struct bitmap *bp);
void bitmap_put(struct bitmap *bp, int index);

#endif