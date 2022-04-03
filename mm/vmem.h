#ifndef VMEM_H
#define VMEM_H

#include "mm/paging.h"
#include "arch/x86.h"

// Virtual address space abstraction
typedef struct vmem
{
    pml4_t* pml4;
} vmem_t;

/**
 * Initializes vmem structure from current address space.
 * 
 * \param vm Structure to initialize
 */
void vmem_init_from_current(vmem_t* vm);

/**
 * Switches to the specified address space.
 * 
 * \param vm Address space
 */
void vmem_switch_to(vmem_t* vm);

/**
 * Allocates memory region in given address space
 * 
 * \param vm Address space
 * \param virt_addr Region start address
 * \param pgcnt Region size in pages
 */
void vmem_alloc_pages(vmem_t* vm, void* virt_addr, size_t pgcnt);

#endif
