#ifndef __KERNEL_LOCK_H__
#define __KERNEL_LOCK_H__

#include <stdint.h>

/**
 * @brief A simple lock for kernel
 * 
 */
struct simplock {
    uint32_t owners;
};

#define SIMPLOCK_INITIALIZER {.owners = 0}

void simplock_obtain(struct simplock *lock);
void simplock_release(struct simplock *lock);

#endif