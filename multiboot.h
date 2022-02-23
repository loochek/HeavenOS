#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "common.h"

#define MB_MEMMAP_TYPE_RAM       1
#define MB_MEMMAP_TYPE_ACPI      3
#define MB_MEMMAP_TYPE_HIBER     4
#define MB_MEMMAP_TYPE_DEFECTIVE 5

typedef struct __attribute__((packed)) mb_tag_header
{
    uint32_t type;
    uint32_t size;
} mb_tag_header_t;

typedef struct __attribute__((packed)) mb_fb_info
{
    mb_tag_header_t header;

    uint8_t  *addr;
    uint32_t  pitch;
    uint32_t  width;
    uint32_t  heigth;
    uint8_t   depth;
    uint8_t   type;
    uint8_t   reserved;

    uint8_t red_field_pos;
    uint8_t red_mask_size;
    uint8_t green_field_pos;
    uint8_t green_mask_size;
    uint8_t blue_field_pos;
    uint8_t blue_mask_size;
} mb_fb_info_t;

typedef struct __attribute__((packed)) mb_memmap_info
{
    mb_tag_header_t header;

    uint32_t entry_size;
    uint32_t entry_version;
} mb_memmap_info_t;

typedef struct __attribute__((packed)) mb_memmap_entry
{
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} mb_memmap_entry_t;

typedef struct mb_memmap_iter
{
    mb_memmap_entry_t *curr_entry;
    int remaining_entries;
} mb_memmap_iter_t;

extern mb_fb_info_t *mb_fb_info;
extern mb_memmap_info_t *mb_memmap_info;

/**
 * Parses Multiboot boot information and fills global structures mb_fb_info, ...
 */
void mb_parse_boot_info();

/**
 * Creates new iterator over memory map regions provided by bootloader. 
 * Requires mb_memmap_info to be filled, panics otherwise
 * 
 * \param it Where to store iterator
 */
void mb_memmap_iter_init(mb_memmap_iter_t *it);

/**
 * Returns next memory region from given iterator. 
 * If there are no more regions, NULL is returned instead
 * 
 * \param it Iterator
 */
mb_memmap_entry_t *mb_memmap_iter_next(mb_memmap_iter_t *it);

#endif