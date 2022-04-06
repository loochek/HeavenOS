#include "arch/x86.h"
#include "kernel/panic.h"
#include "mm/vmem.h"
#include "mm/frame_alloc.h"
#include "mm/obj.h"

OBJ_ALLOC_DEFINE(vmem_area_alloc, vmem_area_t);

static vmem_t *curr_vmem = NULL;

static bool vmem_is_mapped (vmem_t* vm, uint64_t addr);
static bool vmem_intersects(vmem_t* vm, uint64_t other_start_addr, uint64_t other_size);
static void vmem_map_page  (vmem_t* vm, void* virt_addr, void* frame);

void vmem_alloc_pages(vmem_t* vm, void* virt_addr, size_t pgcnt)
{
    kassert_dbg(((uint64_t)virt_addr & (~(PAGE_SIZE - 1))) == (uint64_t)virt_addr);

    if (vmem_intersects(vm, (uint64_t)virt_addr, pgcnt))
        panic("intersection of virtual regions has occured in vmem at %p", vm);

    vmem_area_t *new_area = object_alloc(&vmem_area_alloc);
    new_area->start = (uint64_t)virt_addr;
    new_area->size = pgcnt;
    list_init(&new_area->node);
    list_insert_after(&vm->areas_list->node, &new_area->node);
}

void vmem_init_from_current(vmem_t* vm)
{
    vm->areas_list = object_alloc(&vmem_area_alloc);
    list_init(&vm->areas_list->node);
    vm->pml4 = PHYS_TO_VIRT(x86_read_cr3());
    curr_vmem = vm;
}

void vmem_switch_to(vmem_t* vm)
{
    x86_write_cr3((uint64_t)VIRT_TO_PHYS(vm->pml4));
    curr_vmem = vm;
}

bool vmem_handle_pf(void* fault_addr)
{
    if (vmem_is_mapped(curr_vmem, (uint64_t)fault_addr))
    {
        void* frame = frame_alloc();
        vmem_map_page(curr_vmem, ROUNDDOWN(fault_addr, PAGE_SIZE), frame);
        return true;
    }
    
    return false;
}

static bool vmem_is_mapped(vmem_t* vm, uint64_t addr)
{
    list_node_t *area_node = vm->areas_list->node.next;
    while (area_node != &vm->areas_list->node)
    {
        vmem_area_t *area = (vmem_area_t*)area_node;
        if (area->start <= addr && addr < area->start + area->size * PAGE_SIZE)
            return true;

        area_node = area_node->next;
    }

    return false;
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

static void vmem_map_page(vmem_t* vm, void* virt_addr, void* frame)
{
    uint64_t pml4e = vm->pml4->entries[PML4E_FROM_ADDR(virt_addr)];
    pdpt_t* pdpt = NULL;
    if (!(pml4e & PTE_PRESENT))
    {
        pdpt = frames_alloc(1);
        pml4e = vm->pml4->entries[PML4E_FROM_ADDR(virt_addr)] = (uint64_t)VIRT_TO_PHYS(pdpt) | PTE_PRESENT;
    }
    else
    {
        pdpt = PHYS_TO_VIRT(PTE_ADDR(pml4e));
    }

    uint64_t pdpe = pdpt->entries[PDPE_FROM_ADDR(virt_addr)];
    pgdir_t* pgdir = NULL;
    if (!(pdpe & PTE_PRESENT))
    {
        pgdir = frames_alloc(1);
        pdpe = pdpt->entries[PDPE_FROM_ADDR(virt_addr)] = (uint64_t)VIRT_TO_PHYS(pgdir) | PTE_PRESENT;
    }
    else
    {
        pgdir = PHYS_TO_VIRT(PTE_ADDR(pdpe));
    }

    uint64_t pde = pgdir->entries[PDE_FROM_ADDR(virt_addr)];
    pgtbl_t* pgtbl = NULL;
    if (!(pde & PTE_PRESENT))
    {
        pgtbl = frames_alloc(1);
        pdpe = pgdir->entries[PDE_FROM_ADDR(virt_addr)] = (uint64_t)VIRT_TO_PHYS(pgtbl) | PTE_PRESENT;
    }
    else
    {
        pgtbl = PHYS_TO_VIRT(PTE_ADDR(pde));
    }

    pgtbl->entries[PTE_FROM_ADDR(virt_addr)] = (uint64_t)VIRT_TO_PHYS(frame) | PTE_PRESENT;
}
