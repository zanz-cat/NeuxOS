#include <stdlib.h>
#include <errno.h>

#include "bitmap.h"

#define BITMAP_SET(bitmap, index) bitmap[(index)/8] |= (1 << ((index)%8))
#define BITMAP_CLR(bitmap, index) bitmap[(index)/8] &= (~(1 << ((index)%8)))
#define BITMAP_GET(bitmap, index) (bitmap[(index)/8] & (1 << ((index)%8)))

size_t bitmap_msize(size_t bitsize)
{
    if (bitsize % 8 != 0) {
        return 0;
    }
    return bitsize / 8;
}

void bitmap_init(struct bitmap *bp, size_t bitsize)
{
    int i;
    bp->size = bitsize;
    for (i = 0; i < bitsize / 8; i++) {
        bp->bits[i] = 0;
    }
}

int bitmap_get(struct bitmap *bp)
{
    int i;
    for (i = 0; i < bp->size && BITMAP_GET(bp->bits, i); i++);
    if (i > bp->size - 1) {
        return -ENOMEM;
    }

    BITMAP_SET(bp->bits, i);
    return i;
}

void bitmap_put(struct bitmap *bp, int index)
{
    if (index > bp->size - 1) {
        return;
    }

    BITMAP_CLR(bp->bits, index);
}