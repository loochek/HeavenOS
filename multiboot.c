#include "common.h"
#include "panic.h"
#include "multiboot.h"

#define MB_TAG_ALIGNMENT 8

#define MB_TAG_END     0
#define MB_TAG_MEMMAP  6
#define MB_TAG_FB      8
#define MB_TAG_RSDP_V1 14
#define MB_TAG_RSDP_V2 15

// To be filled by entry point code
extern uint8_t *mb_boot_info;

mb_fb_info_t     *mb_fb_info      = NULL;
mb_memmap_info_t *mb_memmap_info  = NULL;
mb_rsdp_t        *mb_acpi_rsdp_v1 = NULL;
mb_rsdp_t        *mb_acpi_rsdp_v2 = NULL;

void mb_parse_boot_info()
{
    kassert(mb_boot_info != NULL);

    // Skip fixed part
    uint8_t *iter = mb_boot_info + sizeof(mb_tag_header_t);

    // Iterate through boot info tags
    bool should_stop = false;
    while (!should_stop)
    {
        mb_tag_header_t *curr_tag = (mb_tag_header_t*)iter;

        switch (curr_tag->type)
        {
        case MB_TAG_END:
            // End
            should_stop = true;
            break;

        case MB_TAG_MEMMAP:
            // Memory map
            mb_memmap_info = (mb_memmap_info_t*)curr_tag;
            break;

        case MB_TAG_FB:
            // Framebuffer
            mb_fb_info = (mb_fb_info_t*)curr_tag;
            break;

        case MB_TAG_RSDP_V1:
            // ACPI 1.0 RSDP
            mb_acpi_rsdp_v1 = (mb_rsdp_t*)curr_tag;
            break;

        case MB_TAG_RSDP_V2:
            // ACPI 2.0 RSDP
            mb_acpi_rsdp_v2 = (mb_rsdp_t*)curr_tag;
            break;
        }

        iter = ROUNDUP(iter + curr_tag->size, MB_TAG_ALIGNMENT);
    }
}

void mb_memmap_iter_init(mb_memmap_iter_t *it)
{
    kassert(mb_memmap_info != NULL);
    kassert_dbg(it != NULL);

    it->curr_entry = (mb_memmap_entry_t*)(mb_memmap_info + 1);
}

mb_memmap_entry_t *mb_memmap_iter_next(mb_memmap_iter_t *it)
{
    kassert_dbg(it != NULL);
    
    if ((uint8_t*)it->curr_entry >= (uint8_t*)mb_memmap_info + mb_memmap_info->header.size)
        return NULL;

    mb_memmap_entry_t *ret = it->curr_entry;
    it->curr_entry = (mb_memmap_entry_t*)((uint8_t*)it->curr_entry + mb_memmap_info->entry_size);
    return ret;
}
