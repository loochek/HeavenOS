#ifndef X86_H
#define X86_H

#include "common.h"

static inline uint64_t x86_read_cr2()
{
    uint64_t ret;
    asm volatile
    (
        "mov %%cr2, %0"
        : "=r"(ret)
    );
    
    return ret;
}

static inline void outb(uint16_t port, uint8_t data)
{
    asm volatile("outb %0, %1" : : "a" (data), "d" (port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a" (ret) : "d" (port));
    return ret;
}

#endif
