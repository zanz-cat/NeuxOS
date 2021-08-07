#ifndef __LIB_UTILS_H__
#define __LIB_UTILS_H__

#define max(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    (void) (&_a == &_b); \
    _a > _b ? _a : _b; \
})

#define min(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    (void) (&_a == &_b); \
    _a < _b ? _a : _b; \
})

#define intlen(n, radix) ({ \
    typeof(n) _n = (n); \
    typeof(radix) _r = (radix); \
    size_t _len; \
    for (_len = 1; _n /= _r; _len++); \
    _len; \
})

#define arraylen(a) (sizeof(a)/sizeof(a[0]))
#endif