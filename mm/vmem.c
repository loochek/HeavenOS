#include "arch/x86/x86.h"
#include "kernel/panic.h"
#include "mm/vmem.h"
#include "mm/frame_alloc.h"
#include "mm/obj.h"

OBJ_ALLOC_DEFINE(vmem_area_alloc, vmem_area_t);

static vmem_t *curr_vmem = NULL;

static uint64_t vmem_convert_flags(uint64_t flags);
static vmem_area_t *vmem_is_mapped(vmem_t* vm, uint64_t addr);
static bool vmem_intersects(vmem_t* vm, uint64_t other_start_addr, uint64_t other_size);
static void vmem_unmap_page (vmem_t* vm, void* virt_addr);
static void* vmem_ensure_next_table(pte_t* tbl, size_t idx, uint64_t raw_flags);
static pml4_t* vmem_clone_page_table(pml4_t *src_pml4);

int vmem_alloc_pages(vmem_t* vm, void* virt_addr, size_t pgcnt, uint64_t flags)
{
    kassert_dbg(((uint64_t)virt_addr & (~(PAGE_SIZE - 1))) == (uint64_t)virt_addr);

    if (vmem_intersects(vm, (uint64_t)virt_addr, pgcnt))
        panic("intersection of virtual regions has occured in vmem at %p", vm);

    vmem_area_t *new_area = object_alloc(&vmem_area_alloc);
    if (new_area == NULL)
        return -ENOMEM;

    new_area->start = (uint64_t)virt_addr;
    new_area->size = pgcnt;
    new_area->flags = flags;
    list_init(&new_area->node);
    list_insert_after(&vm->areas_list->node, &new_area->node);
    return 0;
}

void vmem_free_pages(vmem_t* vm, void* virt_addr, size_t pgcnt)
{
    kassert_dbg(((uint64_t)virt_addr & (~(PAGE_SIZE - 1))) == (uint64_t)virt_addr);

    uint64_t start_addr = (uint64_t)virt_addr;
    uint64_t end_addr   = start_addr + PAGE_SIZE * pgcnt;

    list_node_t *area_node = vm->areas_list->node.next;
    bool found = false;
    while (area_node != &vm->areas_list->node)
    {
        vmem_area_t *area = (vmem_area_t*)area_node;
        if (start_addr == area->start && area->start + area->size * PAGE_SIZE == end_addr)
        {
            found = true;
            break;
        }

        area_node = area_node->next;
    }

    if (!found)
        panic("alloc-free mismatch in vmem at %p", vm);

    list_extract(area_node);
    object_free(&vmem_area_alloc, area_node);

    for (uint64_t addr = start_addr; addr < end_addr; addr += PAGE_SIZE)
        vmem_unmap_page(vm, (void*)addr);
}

// TODO: check
int vmem_init(vmem_t* vm)
{
    vm->pml4 = frame_alloc();
    if (vm->pml4 == NULL)
        return -ENOMEM;

    vm->areas_list = object_alloc(&vmem_area_alloc);
    list_init(&vm->areas_list->node);
    return 0;
}

int vmem_init_from_current(vmem_t* vm)
{
    vm->areas_list = object_alloc(&vmem_area_alloc);
    list_init(&vm->areas_list->node);
    vm->pml4 = PHYS_TO_VIRT(x86_read_cr3());
    /// TODO: clone
    // vm->pml4 = vmem_clone_page_table(PHYS_TO_VIRT(x86_read_cr3()));
    if (vm->pml4 == NULL)
        return -ENOMEM;

    return 0;
}

// TODO: check
int vmem_clone(vmem_t* dst, vmem_t* src)
{
    // Clone areas list
    dst->areas_list = object_alloc(&vmem_area_alloc);
    list_init(&dst->areas_list->node);

    list_node_t *area_node = src->areas_list->node.next;
    while (area_node != &src->areas_list->node)
    {
        vmem_area_t *area = (vmem_area_t*)area_node;
        vmem_area_t *copy = object_alloc(&vmem_area_alloc);
        copy->start = area->start;
        copy->size  = area->size;
        copy->flags = area->flags;
        list_init(&copy->node);
        list_insert_after(&dst->areas_list->node, &copy->node);

        area_node = area_node->next;
    }

    // Clone page table
    dst->pml4 = vmem_clone_page_table(src->pml4);
    if (dst->pml4 == NULL)
        return -ENOMEM;

    return 0;
}

void vmem_switch_to(vmem_t* vm)
{
    x86_write_cr3((uint64_t)VIRT_TO_PHYS(vm->pml4));
    curr_vmem = vm;
}

// TODO: check
void vmem_destroy(vmem_t* vm)
{
    // Free page table
    for (size_t pml4ei = 0; pml4ei < 512; pml4ei++)
    {
        pte_t pml4e = vm->pml4->entries[pml4ei];
        if (!(pml4e & PTE_PRESENT))
            continue;

        pdpt_t *pdpt = PHYS_TO_VIRT(PTE_ADDR(pml4e));
        for (size_t pdpei = 0; pdpei < 512; pdpei++)
        {
            pte_t pdpte = pdpt->entries[pdpei];
            if (!(pdpte & PTE_PRESENT))
                continue;

            if (!(pdpte & PTE_PAGE_SIZE))
            {
                pgdir_t *pgdir = PHYS_TO_VIRT(PTE_ADDR(pdpte));
                for (int pdei = 0; pdei < 512; pdei++)
                {
                    pte_t pgdire = pgdir->entries[pdei];
                    if (!(pgdire & PTE_PRESENT))
                        continue;

                    if (!(pgdire & PTE_PAGE_SIZE))
                        frame_free(PTE_ADDR(pgdire));
                }

                frame_free(PTE_ADDR(pdpte));
            }
        }

        frame_free(PTE_ADDR(pml4e));
    }

    frame_free(vm->pml4);

    // Free areas list
    while (list_empty(&vm->areas_list->node))
    {
        list_node_t *to_pop = vm->areas_list->node.next;
        list_extract(to_pop);
        object_free(&vmem_area_alloc, to_pop);
    }

    object_free(&vmem_area_alloc, vm->areas_list);
}

int vmem_map_page(vmem_t* vm, void* virt_addr, void* frame, uint64_t flags)
{
    kassert_dbg(((uint64_t)virt_addr & (~(PAGE_SIZE - 1))) == (uint64_t)virt_addr);
    kassert_dbg(((uint64_t)frame & (~(PAGE_SIZE - 1))) == (uint64_t)frame);

    flags = vmem_convert_flags(flags);

    pdpt_t* pdpt = vmem_ensure_next_table(vm->pml4->entries, PML4E_FROM_ADDR(virt_addr), flags);
    if (pdpt == NULL)
        return -ENOMEM;

    pgdir_t* pgdir = vmem_ensure_next_table(pdpt->entries, PDPE_FROM_ADDR(virt_addr), flags);
    if (pgdir == NULL)
        return -ENOMEM;

    pgtbl_t* pgtbl = vmem_ensure_next_table(pgdir->entries, PDPE_FROM_ADDR(virt_addr), flags);
    if (pgtbl == NULL)
        return -ENOMEM;

    kassert(!(pgtbl->entries[PTE_FROM_ADDR(virt_addr)] & PTE_PRESENT));
    pgtbl->entries[PTE_FROM_ADDR(virt_addr)] = (uint64_t)frame | PTE_PRESENT | flags;
    return 0;
}

int vmem_map_page_2mb(vmem_t* vm, void* virt_addr, void* frame, uint64_t flags)
{
    kassert_dbg(((uint64_t)virt_addr & (~(2 * MB - 1))) == (uint64_t)virt_addr);
    kassert_dbg(((uint64_t)frame & (~(2 * MB - 1))) == (uint64_t)frame);

    flags = vmem_convert_flags(flags);

    pdpt_t* pdpt = vmem_ensure_next_table(vm->pml4->entries, PML4E_FROM_ADDR(virt_addr), flags);
    if (pdpt == NULL)
        return -ENOMEM;

    pgdir_t* pgdir = vmem_ensure_next_table(pdpt->entries, PDPE_FROM_ADDR(virt_addr), flags);
    if (pgdir == NULL)
        return -ENOMEM;

    uint64_t pde = pgdir->entries[PDE_FROM_ADDR(virt_addr)];
    kassert(!(pde & PTE_PRESENT) && !(pde & PTE_PAGE_SIZE));
    pgdir->entries[PDE_FROM_ADDR(virt_addr)] = (uint64_t)frame | PTE_PRESENT | PTE_PAGE_SIZE | flags;
    return 0;
}

int vmem_map_page_1gb(vmem_t* vm, void* virt_addr, void* frame, uint64_t flags)
{
    kassert_dbg(((uint64_t)virt_addr & (~(GB - 1))) == (uint64_t)virt_addr);
    kassert_dbg(((uint64_t)frame & (~(GB - 1))) == (uint64_t)frame);

    flags = vmem_convert_flags(flags);

    pdpt_t* pdpt = vmem_ensure_next_table(vm->pml4->entries, PML4E_FROM_ADDR(virt_addr), flags);
    if (pdpt == NULL)
        return -ENOMEM;
    
    uint64_t pdpe = pdpt->entries[PDPE_FROM_ADDR(virt_addr)];
    kassert(!((pdpe & PTE_PRESENT) && !(pdpe & PTE_PAGE_SIZE)));
    pdpt->entries[PDPE_FROM_ADDR(virt_addr)] = (uint64_t)frame | PTE_PRESENT | PTE_PAGE_SIZE | flags;
    return 0;
}

bool vmem_handle_pf(void* fault_addr)
{
    vmem_area_t *area = vmem_is_mapped(curr_vmem, (uint64_t)fault_addr);
    if (!area)
        return false;

    void* frame = frame_alloc();
    if (!frame)
        panic("Can't map page: out of memory");

    int status = vmem_map_page(curr_vmem, ROUNDDOWN(fault_addr, PAGE_SIZE), frame, area->flags);
    if (status < 0)
        panic("Can't map page: %i", status);

    return true;
}

static uint64_t vmem_convert_flags(uint64_t flags)
{
    uint64_t pte_flags = 0;
    if (flags & VMEM_USER)
        pte_flags |= PTE_USER;

    if (flags & VMEM_WRITE)
        pte_flags |= PTE_WRITEABLE;

    return pte_flags;
}

static vmem_area_t *vmem_is_mapped(vmem_t* vm, uint64_t addr)
{
    list_node_t *area_node = vm->areas_list->node.next;
    while (area_node != &vm->areas_list->node)
    {
        vmem_area_t *area = (vmem_area_t*)area_node;
        if (area->start <= addr && addr < area->start + area->size * PAGE_SIZE)
            return area;

        area_node = area_node->next;
    }

    return NULL;
}

static bool vmem_intersects(vmem_t* vm, uint64_t other_start_addr, uint64_t other_size)
{
    uint64_t other_end_addr = other_start_addr + other_size;

    list_node_t *area_node = vm->areas_list->node.next;
    while (area_node != &vm->areas_list->node)
    {
        vmem_area_t *area = (vmem_area_t*)area_node;
        if (!(other_end_addr <= area->start || area->start + area->size * PAGE_SIZE <= other_start_addr))
            return true;

        area_node = area_node->next;
    }

    return false;
}

static void vmem_unmap_page(vmem_t* vm, void* virt_addr)
{
    uint64_t pml4e = vm->pml4->entries[PML4E_FROM_ADDR(virt_addr)];
    if (!(pml4e & PTE_PRESENT))
        return;

    pdpt_t*  pdpt = PHYS_TO_VIRT(PTE_ADDR(pml4e));
    uint64_t pdpe = pdpt->entries[PDPE_FROM_ADDR(virt_addr)];
    if (!(pdpe & PTE_PRESENT))
        return;

    pgdir_t* pgdir = PHYS_TO_VIRT(PTE_ADDR(pdpe));
    uint64_t pde = pgdir->entries[PDE_FROM_ADDR(virt_addr)];
    if (!(pde & PTE_PRESENT))
        return;

    pgtbl_t* pgtbl = PHYS_TO_VIRT(PTE_ADDR(pde));
    pgtbl->entries[PTE_FROM_ADDR(virt_addr)] = 0;
}

static void* vmem_ensure_next_table(pte_t* tbl, size_t idx, uint64_t raw_flags)
{
    pte_t pte = tbl[idx];
    void* next_tbl = NULL;
    if (pte & PTE_PRESENT)
    {
        next_tbl = PHYS_TO_VIRT(PTE_ADDR(pte));
        tbl[idx] |= raw_flags;
    }
    else
    {
        next_tbl = frame_alloc();
        if (next_tbl == NULL)
        {
            return NULL;
        }

        memset(next_tbl, 0, PAGE_SIZE);
        tbl[idx] = (uint64_t)VIRT_TO_PHYS(next_tbl) | PTE_PRESENT | raw_flags;
    }

    return next_tbl;
}

static pml4_t* vmem_clone_page_table(pml4_t *src_pml4)
{
    /// TODO:
    (void)src_pml4;
    return NULL;
}
