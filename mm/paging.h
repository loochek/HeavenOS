#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "mm/mem_layout.h"

#define PTE_PAGE_SIZE (1ull << 7)
#define PTE_PRESENT   (1ull << 0)

#define PTE_FLAGS_MASK ((1ull << 12) - 1)
#define PTE_ADDR_MASK  ((1ull << 48) - 1)
#define PTE_ADDR(pte) ((void*)(((pte) & PTE_ADDR_MASK) & ~PTE_FLAGS_MASK))

#define ATTR_ALIGN_4096 __attribute__((aligned(4096)))

#define PML4E_FROM_ADDR(addr) (((uint64_t)(addr) >> 39) & 0x1ff)
#define PDPE_FROM_ADDR(addr)  (((uint64_t)(addr) >> 30) & 0x1ff)
#define PDE_FROM_ADDR(addr)   (((uint64_t)(addr) >> 21) & 0x1ff)
#define PTE_FROM_ADDR(addr)   (((uint64_t)(addr) >> 12) & 0x1ff)

#define PHYS_TO_VIRT(addr) ((void*)(KERNEL_DIRECT_PHYS_MAPPING_START + (uint64_t)(addr)))
#define VIRT_TO_PHYS(addr) ((void*)((uint64_t)(addr) - KERNEL_DIRECT_PHYS_MAPPING_START))

typedef struct pml4
{
    uint64_t entries[512];
} ATTR_ALIGN_4096 pml4_t;

typedef struct pdpt
{
    uint64_t entries[512];
} ATTR_ALIGN_4096 pdpt_t;

typedef struct pgdir
{
    uint64_t entries[512];
} ATTR_ALIGN_4096 pgdir_t;

typedef struct pgtbl
{
    uint64_t entries[512];
} ATTR_ALIGN_4096 pgtbl_t;

#endif
