#include "boot/early.h"
#include "kernel/multiboot.h"
#include "kernel/console.h"
#include "kernel/printk.h"
#include "kernel/panic.h"
#include "kernel/irq.h"
#include "drivers/fb.h"
#include "drivers/acpi.h"
#include "drivers/apic.h"
#include "mm/paging.h"
#include "mm/frame_alloc.h"
#include "mm/vmem.h"

void dump_memmap()
{
    struct mb_memmap_iter memmap_iter;
    mb_memmap_iter_init(&memmap_iter);

    printk("Multiboot memory map:\n");

    struct mb_memmap_entry* memmap_entry;
    while ((memmap_entry = mb_memmap_iter_next(&memmap_iter)) != NULL)
    {
        printk("mem region: ");
        switch (memmap_entry->type)
        {
        case MB_MEMMAP_TYPE_RAM:
            printk("[RAM]       ");
            break;

        case MB_MEMMAP_TYPE_ACPI:
            printk("[ACPI]      ");
            break;

        case MB_MEMMAP_TYPE_HIBER:
            printk("[HIBER]     ");
            break;

        case MB_MEMMAP_TYPE_DEFECTIVE:
            printk("[DEFECTIVE] ");
            break;

        default:
            printk("[RESERVED]  ");
            break;
        }

        printk("%p - %p len=0x%x\n", memmap_entry->base_addr, memmap_entry->base_addr + memmap_entry->length,
            memmap_entry->length);
    }
}

extern pml4_t early_pml4;

void unmap_early()
{
    pml4_t *pml4_virt = PHYS_TO_VIRT(&early_pml4);
    pdpt_t *pdpd_virt = PHYS_TO_VIRT(pml4_virt->entries[0] & (~0xFFF));
    pdpd_virt->entries[0] = 0;
}

void kmain(early_data_t *early_data)
{
    // unmap_early();
    irq_init();
    mb_parse_boot_info(early_data);
    fb_init(mb_fb_info);
    cons_init();

    printk("HeavenOS version %s\n", HEAVENOS_VERSION);
    acpi_init();
    apic_init();
    apic_setup_timer();

    dump_memmap();
    frame_alloc_init();

    // Frame allocator test
    // for (int i = 0; i < 522240; i++)
    // {
    //     kassert(frame_alloc());
    // }

    // Lazy allocation test
    vmem_t vmem;
    vmem_init_from_current(&vmem);
    vmem_alloc_pages(&vmem, (void*)0x40000000, 262144);

    irq_enable();

    for (int i = 0; i < 20; i += 2)
        *(volatile int*)(0x40000000 + (uint64_t)i * PAGE_SIZE) = 0xAAA; // Success

    *(volatile int*)((uint64_t)0x40000000 + 0x40000000 - PAGE_SIZE) = 0xAAA; // Success
    *(volatile int*)((uint64_t)0x40000000 + 0x40000000) = 0xDEAD; // Fault

    // Lazy allocation test 2
    // vmem_t vmem;
    // vmem_init_from_current(&vmem);
    // vmem_alloc_pages(&vmem, (void*)0x40000000, 1);
    // vmem_alloc_pages(&vmem, (void*)0x40002000, 1);

    // *(volatile int*)(0x40000000) = 0xAAA; // Success
    // *(volatile int*)(0x40000000 + (uint64_t)2 * PAGE_SIZE) = 0xAAA; // Success
    // *(volatile int*)(0x40000000 + PAGE_SIZE) = 0xAAA; // Fault

    panic("manually initiated %s", "panic");
}
