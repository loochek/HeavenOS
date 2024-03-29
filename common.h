#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

/// HeavenOS version to display
#define HEAVENOS_VERSION "hw4-dev"

#define PAGE_SIZE 4096

#define CACHE_LINE_SIZE_BYTES 64

#define GB (1 << 30)
#define MB (1 << 20)
#define KB (1 << 10)

/* Round down to the nearest multiple of n */
#define ROUNDDOWN(a, n) ({        \
    uint64_t __a = (uint64_t)(a); \
    (typeof(a))(__a - __a % (n)); })

/* Round up to the nearest multiple of n */
#define ROUNDUP(a, n) ({                                  \
    uint64_t __n = (uint64_t)(n);                         \
    (typeof(a))(ROUNDDOWN((uint64_t)(a) + __n - 1, __n)); })

#define ALIGN_UP(X, R) ((((uint64_t)X) + (R) - 1) / (R) * (R))
#define DIV_ROUNDUP(X, R) ((((uint64_t)X) + (R) - 1) / (R))

#define GET_BIT(val, bit) (((val) >> (bit)) & 1)
#define SET_BIT(val, bit) ((val) | (1 << (bit)))
#define CLEAR_BIT(val, bit) ((val) & (~(1 << (bit))))
#define FLIP_BIT(val, bit) ((val) ^ (1 << (bit)))

#define UNUSED(x) (void)(x)

// Error codes

#define ENOSYS 1
#define ENOMEM 2
#define EINVAL 3
#define ECHILD 4

typedef struct mem_region
{
    void* start;
    void* end;
} mem_region_t;

static inline int memcmp(const void* str1, const void* str2, size_t count)
{
    const uint8_t *s1 = (const uint8_t*)str1;
    const uint8_t *s2 = (const uint8_t*)str2;

    while (count-- > 0)
    {
        if (*s1++ != *s2++)
        {
            return s1[-1] < s2[-1] ? -1 : 1;
        }
    }
    return 0;
}

static inline void memset(void* p, int ch, size_t sz)
{
    uint8_t* ptr = (uint8_t*)p;
    while (sz > 0)
    {
        *ptr = ch;
        ptr++;
        sz--;
    }
}

static inline void memcpy(void* dst, void* src, size_t sz)
{
    uint8_t* d = dst;
    const uint8_t* s = src;
    while (sz > 0)
    {
        *d = *s;
        d++;
        s++;
        sz--;
    }
}

static inline void memmove(void *dst, const void *src, size_t n)
{
    const uint8_t *s = src;
    uint8_t *d = dst;

    if (s < d && s + n > d) {
        s += n;
        d += n;
        if (!(((intptr_t)s & 7) | ((intptr_t)d & 7) | (n & 7))) {
            __asm__ volatile("std; rep movsq\n" ::"D"(d - 8), "S"(s - 8), "c"(n / 8)
                         : "cc", "memory");
        } else {
            __asm__ volatile("std; rep movsb\n" ::"D"(d - 1), "S"(s - 1), "c"(n)
                         : "cc", "memory");
        }
        /* Some versions of GCC rely on DF being clear */
        __asm__ volatile("cld" ::
                             : "cc");
    } else {
        if (!(((intptr_t)s & 7) | ((intptr_t)d & 7) | (n & 7))) {
            __asm__ volatile("cld; rep movsq\n" ::"D"(d), "S"(s), "c"(n / 8)
                         : "cc", "memory");
        } else {
            __asm__ volatile("cld; rep movsb\n" ::"D"(d), "S"(s), "c"(n)
                         : "cc", "memory");
        }
    }
}

static inline size_t strlen(const char* str)
{
	size_t len = 0;
	while (str[len])
    {
		len++;
    }
	return len;
}

#endif
