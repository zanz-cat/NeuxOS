#include "sched.h"
#include "lock.h"

void simplock_obtain(struct simplock *lock)
{
    while (lock->owners > 0) {
        if (eflags() & EFLAGS_IF) {
            yield();
        }
    }
    lock->owners++;
}

void simplock_release(struct simplock *lock)
{
    lock->owners--;
}