#ifndef __MISC_MISC_H__
#define __MISC_MISC_H__

#include <stdint.h>

#include <config.h>
#include <arch/x86.h>

#include <kernel/memory.h>

struct share_data {
    uint32_t kernel_start; // physical addr
    uint32_t kernel_end;
    uint32_t ards_cnt;
    struct ARDS ards[0];
} __attribute__((packed));

#ifdef NO_PAGE
    #define SHARE_DATA() ((struct share_data *)CONFIG_SHARE_DATA_ADDR)
#else
    #define SHARE_DATA() ((struct share_data *)virt_addr(CONFIG_SHARE_DATA_ADDR))
#endif

#define max(a, b) \
    __extension__ ({ \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        (void) (&_a == &_b); \
        _a > _b ? _a : _b; \
    })

#define min(a, b) \
    __extension__ ({ \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        (void) (&_a == &_b); \
        _a < _b ? _a : _b; \
    })

#define intlen(n, radix) \
    __extension__ ({ \
        typeof(n) _n = (n); \
        typeof(radix) _r = (radix); \
        size_t _len; \
        for (_len = 1; _n /= _r; _len++); \
        _len; \
    })

#define arraylen(a) (sizeof(a)/sizeof(a[0]))

#define PTR_DIFF(ptr1, ptr2) \
    ((uint32_t)(ptr1) - (uint32_t)(ptr2))

#define PTR_ADD(ptr, n) \
    ((void *)((uint32_t)(ptr) + (n)))

#define PTR_SUB(ptr, n) \
    ((void *)((uint32_t)(ptr) - (n)))

#define ALIGN_FLOOR(n, align) \
    (typeof(n)) ((n) & (~((typeof(n))((align) - 1))))

#define ALIGN_CEIL(n, align) \
    ALIGN_FLOOR(((n) + ((typeof(n))(align) - 1)), align)

#define PTR_ALIGN_FLOOR(ptr, align) \
    (typeof(ptr))ALIGN_FLOOR((uint32_t)ptr, align)

#define PTR_ALIGN_CEIL(ptr, align) \
    PTR_ALIGN_FLOOR((typeof(ptr))PTR_ADD(ptr, (align) - 1), align)

#define container_of(ptr, type, member) \
__extension__({ \
    void *__mptr = (void *)(ptr); \
    ((type *)(__mptr - offsetof(type, member))); \
})

#define is_power_of_2(n) (n && !(n & (n - 1)))

#define do_div(n, base) \
__extension__({ \
    uint32_t __base = (base); \
    uint32_t __rem; \
    __rem = ((uint64_t)(n)) % __base; \
    (n) = ((uint64_t)(n)) / __base; \
    __rem; \
})

#define ceil_div(n, base) (((n) + ((base) - 1)) / (base))

static inline uint32_t eflags(void)
{
    uint32_t eflags;
    asm volatile("pushf\n\t"
        "pop %%eax":"=a"(eflags)::"memory");
    return eflags;
}

#define SAVE_STATE() \
    asm("pusha\n\t" \
        "push %ds\n\t" \
        "push %es\n\t" \
        "push %fs\n\t" \
        "push %gs\n\t");

#define RESTORE_STATE() \
    asm("pop %gs\n\t" \
        "pop %fs\n\t" \
        "pop %es\n\t" \
        "pop %ds\n\t" \
        "popa\n\t");

#endif