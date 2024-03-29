#include <stdint.h>
#include <stddef.h>

int memcmp(const void* str1, const void* str2, size_t count)
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

void memset(void* p, int ch, size_t sz)
{
    uint8_t* ptr = (uint8_t*)p;
    while (sz > 0)
    {
        *ptr = ch;
        ptr++;
        sz--;
    }
}

void memcpy(void* dst, void* src, size_t sz)
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

void memmove(void *dst, const void *src, size_t n)
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
