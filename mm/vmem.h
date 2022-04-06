#ifndef VMEM_H
#define VMEM_H

#include "common.h"
#include "mm/paging.h"
#include "utils/list.h"

/// Structure which describes continious mapping region
typedef struct vmem_area
{
    list_node_t node;

    uint64_t start;
    size_t size; // In pages
} vmem_area_t;

/// Virtual address space abstraction
typedef struct vmem
{
    vmem_area_t *areas_list;
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

/**
 * Page fault handler for on-demand allocation
 * 
 * \param fault_addr Fault address
 * 
 * \return True if on-demand allocation was performed, false if it's a real page fault
 */
bool vmem_handle_pf(void* fault_addr);

#endif
