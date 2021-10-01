#ifndef __KERNEL_LOCK_H__
#define __KERNEL_LOCK_H__

#include <stdint.h>

struct sample_lock {
    uint32_t owners;
};

void obtain_lock(struct sample_lock *lock);
void release_lock(struct sample_lock *lock);

#endif