#ifndef X86_H
#define X86_H

#include "common.h"

uint64_t x86_read_cr2()
{
    uint64_t ret;
    asm volatile
    (
        "mov %%cr2, %0"
        : "=r"(ret)
    );
    
    return ret;
}

#endif
