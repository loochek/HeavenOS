#ifndef VMEM_H
#define VMEM_H

#include "common.h"
#include "mm/paging.h"
#include "utils/list.h"

#define VMEM_NO_FLAGS 0
#define VMEM_USER     (1 << 0)
#define VMEM_WRITE    (1 << 1)
// Custom bit - used to mark pages that have been allocated by vmem_alloc_pages
// (i.e. must be freed when vmem is destroyed)
#define VMEM_ALLOC (1 << 2)

/// Structure which describes continious mapping region
typedef struct vmem_area
{
    list_node_t node;

    uint64_t start;
    size_t   size; // In pages
    uint64_t flags;
} vmem_area_t;

/// Virtual address space abstraction
typedef struct vmem
{
    vmem_area_t *areas_list;
    pml4_t* pml4;
} vmem_t;

/**
 * Initializes new address space
 * 
 * \param vm Structure to initialize
 */
int vmem_init(vmem_t* vm);

/**
 * Initializes vmem structure from current address space.
 * 
 * \param vm Structure to initialize
 * 
 * \return 0 or error code
 */
int vmem_init_from_current(vmem_t* vm);

/**
 * Clones address space
 * 
 * \param dst Destination vmem structure
 * \param src Source address space
 * 
 * \return 0 or error code
 */
int vmem_clone(vmem_t* dst, vmem_t* src);

/**
 * Switches to the specified address space.
 * 
 * \param vm Address space
 *
 */
void vmem_switch_to(vmem_t* vm);

/**
 * Allocates memory region in given address space
 * 
 * \param vm Address space
 * \param virt_addr Region start address
 * \param pgcnt Region size in pages
 * \param flags Flags
 * 
 * \return 0 or error code
 */
int vmem_alloc_pages(vmem_t* vm, void* virt_addr, size_t pgcnt, uint64_t flags);

/**
 * Frees memory region in given address space
 * 
 * \param vm Address space
 * \param virt_addr Region start address
 * \param pgcnt Region size in pages
 */
void vmem_free_pages(vmem_t* vm, void* virt_addr, size_t pgcnt);

/**
 * Maps given physical frame into given address space
 * 
 * \param vm Address space
 * \param virt_addr Virtual address
 * \param frame Physical address
 * \param flags Flags
 * 
 * \return 0 or error code
 */
int vmem_map_page(vmem_t* vm, void* virt_addr, void* frame, uint64_t flags);

/**
 * Maps given physical 2MB frame into given address space
 * 
 * \param vm Address space
 * \param virt_addr Virtual address
 * \param frame Physical address
 * \param flags Flags
 * 
 * \return 0 or error code
 */
int vmem_map_page_2mb(vmem_t* vm, void* virt_addr, void* frame, uint64_t flags);

/**
 * Maps given physical 1GB frame into given address space
 * 
 * \param vm Address space
 * \param virt_addr Virtual address
 * \param frame Physical address
 * \param flags Flags
 * 
 * \return 0 or error code
 */
int vmem_map_page_1gb(vmem_t* vm, void* virt_addr, void* frame, uint64_t flags);

/**
 * Frees memory occupied by given address space. 
 * Address space becomes not usable anymore.
 * 
 * \param vm Address space
 */
void vmem_destroy(vmem_t* vm);

/**
 * Page fault handler for on-demand allocation
 * 
 * \param fault_addr Fault address
 * 
 * \return True if on-demand allocation was performed, false if it's a real page fault
 */
bool vmem_handle_pf(void* fault_addr);

#endif
