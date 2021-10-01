#include "sched.h"
#include "lock.h"

void obtain_lock(struct sample_lock *lock)
{
    while (lock->owners > 0) {
        if (eflags() & EFLAGS_IF) {
            yield();
        }
    }
    lock->owners++;
}

void release_lock(struct sample_lock *lock)
{
    lock->owners--;
}