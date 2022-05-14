#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "mm/mem_layout.h"

#define PTE_PRESENT   (1ull << 0)
#define PTE_WRITEABLE (1ull << 1)
#define PTE_USER      (1ull << 2)
#define PTE_PAGE_SIZE (1ull << 7)
// Custom bit - used to mark physical frames that have been allocated by vmem_alloc_pages
// (i.e. must be freed when vmem is destroyed)
#define PTE_ALLOC     (1ull << 10)

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

#define MAKE_ADDR(m, p, d, t, o) \
    ((((uint64_t)(m) << 39 | (uint64_t)(p) << 30 | (uint64_t)(d) << 21 | (uint64_t)(t) << 12 | (o))) | ((((m) >> 8) & 0x1) ? ((((uint64_t)1 << 16) - 1) << 48) : 0))

typedef uint64_t pte_t;

typedef struct pml4
{
    pte_t entries[512];
} ATTR_ALIGN_4096 pml4_t;

typedef struct pdpt
{
    pte_t entries[512];
} ATTR_ALIGN_4096 pdpt_t;

typedef struct pgdir
{
    pte_t entries[512];
} ATTR_ALIGN_4096 pgdir_t;

typedef struct pgtbl
{
    pte_t entries[512];
} ATTR_ALIGN_4096 pgtbl_t;

#endif
