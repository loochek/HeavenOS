#include "multiboot.h"
#include "fb.h"
#include "console.h"
#include "printk.h"
#include "panic.h"
#include "acpi.h"
#include "apic.h"
#include "irq.h"

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

void kmain()
{
    irq_init();
    mb_parse_boot_info();
    fb_init(mb_fb_info);
    cons_init();

    printk("HeavenOS version %s\n", HEAVENOS_VERSION);
    acpi_init();
    apic_init();

    dump_memmap();
    printk("\n\n\n\n");

    //irq_enable();

    panic("manually initiated %s", "panic");
}
