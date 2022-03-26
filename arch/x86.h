#ifndef X86_H
#define X86_H

#include "common.h"

static inline uint64_t x86_read_cr2()
{
    uint64_t ret;
    __asm__ volatile
    (
        "mov %%cr2, %0"
        : "=r"(ret)
    );
    
    return ret;
}

static inline void outb(uint16_t port, uint8_t data)
{
    __asm__ volatile("outb %0, %1" : : "a" (data), "d" (port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a" (ret) : "d" (port));
    return ret;
}

static inline uint64_t x86_read_cr3()
{
    uint64_t ret;
    __asm__ volatile
    (
        "mov %%cr3, %0"
        : "=r"(ret)
    );
    return ret;
}

static inline void x86_write_cr3(uint64_t x)
{
    __asm__ volatile
    (
        "mov %0, %%cr3"
        : : "r"(x)
    );
}

#endif
