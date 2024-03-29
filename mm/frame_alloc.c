#include "common.h"
#include "kernel/multiboot.h"
#include "kernel/panic.h"
#include "mm/paging.h"
#include "mm/frame_alloc.h"
#include "utils/list.h"

#define MAX_ORDER 10
#define MAX_ZONE_COUNT 10

typedef struct free_blocks_list
{
    list_node_t free_blocks_head;
    uint8_t *bitmap;
} free_blocks_list_t;

typedef struct allocator_zone
{
    free_blocks_list_t orders[MAX_ORDER + 1]; 
    void* start_addr;
    void* end_addr;
    size_t pages_count;
} allocator_zone_t;

static allocator_zone_t allocator_zones[MAX_ZONE_COUNT] = {0};
static size_t zones_count = 0;

static size_t zone_add(uint64_t addr, size_t pages_count);
static void *zone_alloc(allocator_zone_t *zone, int order);
static void zone_dealloc(allocator_zone_t *zone, uint64_t addr, int order);
static int pages2order(size_t pages);

// Those constants are defined by linker script.
extern int _phys_start_kernel_sections;
extern int _phys_end_kernel_sections;

void frame_alloc_init()
{
    mb_memmap_iter_t mmap_it;
    mb_memmap_iter_init(&mmap_it);
    mb_memmap_entry_t *mmap_entry;
    size_t pgcnt = 0;

    // Avoid preserved regions
    uint64_t reserved_end  = (uint64_t)PHYS_TO_VIRT(&_phys_end_kernel_sections);
    uint64_t reserved_end2 = (uint64_t)mb_memory_region().end;
    if (reserved_end2 > reserved_end)
        reserved_end = reserved_end2;

    reserved_end = ROUNDUP(reserved_end, PAGE_SIZE);

    while ((mmap_entry = mb_memmap_iter_next(&mmap_it)) != NULL)
    {
        if (mmap_entry->type != MB_MEMMAP_TYPE_RAM && mmap_entry->type != MB_MEMMAP_TYPE_HIBER)
            continue;

        uint64_t base_addr = (uint64_t)PHYS_TO_VIRT(mmap_entry->base_addr);
        base_addr = ROUNDUP(base_addr, PAGE_SIZE);
        size_t region_size = mmap_entry->length / PAGE_SIZE;

        if (base_addr < reserved_end)
        {
            // I don't want to bother with preserving
            // a memory region occupied by the kernel an Multiboot info.
            // So just ignore - we've already wasted a lot of memory)0

            if (reserved_end >= base_addr + region_size * PAGE_SIZE)
                continue;

            region_size -= (reserved_end - base_addr) / PAGE_SIZE;
            base_addr = reserved_end;
        }

        if (region_size < ((size_t)1 << MAX_ORDER))
        {
            // Ignore too small zone
            continue;
        }

        // printk("zone_add: region at %p with %d pages\n", base_addr, region_size);
        pgcnt += zone_add(base_addr, region_size);
    }

    printk("Frame allocator initialized with %d frames\n", pgcnt);
}

void* frames_alloc(size_t n) 
{
    int order = pages2order(n);
    for (int i = 0; i < (int)zones_count; i++)
    {
        void* block = zone_alloc(&allocator_zones[i], order);
        if (block != NULL)
            return block;
    }

    return NULL;
}

void frames_free(void* addr, size_t n)
{
    int order = pages2order(n);
    for (int i = 0; i < (int)zones_count; i++)
    {
        if ((uint64_t)allocator_zones[i].start_addr <= (uint64_t)addr &&
            (uint64_t)addr < (uint64_t)allocator_zones[i].end_addr)
        {
            zone_dealloc(&allocator_zones[i], (uint64_t)addr, order);
            return;
        }
    }

    panic("frames_free on unknown address %p", addr);
}

void* frame_alloc()
{
    return frames_alloc(1);
}

void frame_free(void* addr)
{
    return frames_free(addr, 1);
}

static size_t zone_add(uint64_t addr, size_t pages_count)
{
    kassert_dbg((addr & (~(PAGE_SIZE - 1))) == addr);
    kassert_dbg(pages_count > ((size_t)1 << MAX_ORDER));
    kassert_dbg(zones_count < MAX_ZONE_COUNT);

    allocator_zone_t *zone = &allocator_zones[zones_count++];
    zone->start_addr = (void*)addr;

    // 1. Determine max possible bitmap size and reserve space for it

    // In bytes
    size_t required_bitmap_size = 0;
    for (int i = 0; i <= MAX_ORDER; i++)
        required_bitmap_size += DIV_ROUNDUP(pages_count / ((size_t)1 << i) / 2, 8);

    // In pages
    size_t bitmap_size = DIV_ROUNDUP(required_bitmap_size, PAGE_SIZE);
    uint64_t bitmap_ptr = (uint64_t)zone->start_addr;
    zone->start_addr += PAGE_SIZE * bitmap_size;
    pages_count -= bitmap_size;

    // 2. Align remaining part of the zone by the max allocation size
    // (Wasting up to 2 * (1 << MAX_ORDER) pages of out precious memory!)
    zone->start_addr = ROUNDUP  (zone->start_addr,  PAGE_SIZE * ((size_t)1 << MAX_ORDER));
    zone->end_addr   = ROUNDDOWN(zone->start_addr + pages_count * PAGE_SIZE, PAGE_SIZE * ((size_t)1 << MAX_ORDER));

    // Final pages count value!  
    zone->pages_count = ((uint64_t)zone->end_addr - (uint64_t)zone->start_addr) / PAGE_SIZE;

    // 2. Fill bitmaps pointers

    for (int i = 0; i < MAX_ORDER; i++)
    {
        size_t order_bitmap_size = DIV_ROUNDUP(zone->pages_count / ((size_t)1 << MAX_ORDER) / 2, 8);
        zone->orders[i].bitmap = (uint8_t*)bitmap_ptr;
        bitmap_ptr += order_bitmap_size;
        kassert(bitmap_ptr < bitmap_ptr + bitmap_size * PAGE_SIZE);

        list_init(&zone->orders[i].free_blocks_head);
    }

    zone->orders[MAX_ORDER].bitmap = NULL;
    list_init(&zone->orders[MAX_ORDER].free_blocks_head);

    for (size_t i = 0; i < zone->pages_count / ((size_t)1 << MAX_ORDER); i++)
    {
        list_node_t *block_ptr = zone->start_addr + i * ((size_t)1 << MAX_ORDER) * PAGE_SIZE;
        list_init(block_ptr);
        list_insert_after(&zone->orders[MAX_ORDER].free_blocks_head, block_ptr);
    }

    return zone->pages_count;
}

static void *zone_alloc(allocator_zone_t *zone, int order)
{
    kassert_dbg(zone != NULL);
    kassert_dbg(order <= MAX_ORDER);

    list_node_t *order_blocks_head = &zone->orders[order].free_blocks_head;
    if (!list_empty(order_blocks_head))
    {
        // Perfect fit
        list_node_t *block = order_blocks_head->next;
        list_extract(block);

        size_t block_index = ((uint64_t)block - (uint64_t)zone->start_addr) / (((size_t)1 << order) * PAGE_SIZE);
        size_t buddy_index = block_index / 2;
        uint8_t *bitmap = zone->orders[order].bitmap;
        bitmap[buddy_index / 8] = FLIP_BIT(bitmap[buddy_index / 8], buddy_index % 8);
        
        memset(block, 0, PAGE_SIZE * ((size_t)1 << order));
        return (void*)block;
    }

    // Check for larger blocks
    for (int i = order; i <= MAX_ORDER; i++)
    {
        list_node_t *order_blocks_head = &zone->orders[i].free_blocks_head;
        if (list_empty(order_blocks_head))
            continue;

        // Larger block is present, split it

        list_node_t *curr_split_block = order_blocks_head->next;
        list_extract(curr_split_block);

        if (i < MAX_ORDER)
        {
            size_t block_index = ((uint64_t)curr_split_block - (uint64_t)zone->start_addr) / (((size_t)1 << i) * PAGE_SIZE);
            size_t buddy_index = block_index / 2;
            uint8_t *bitmap = zone->orders[i].bitmap;
            bitmap[buddy_index / 8] = FLIP_BIT(bitmap[buddy_index / 8], buddy_index % 8);
        }

        for (int j = i - 1; j >= order; j--)
        {
            size_t block_index = ((uint64_t)curr_split_block - (uint64_t)zone->start_addr) / (((size_t)1 << j) * PAGE_SIZE);
            size_t buddy_index = block_index / 2;
            uint8_t *bitmap = zone->orders[j].bitmap;
            bitmap[buddy_index / 8] = SET_BIT(bitmap[buddy_index / 8], buddy_index % 8);

            list_node_t *first_half  = curr_split_block;
            list_node_t *second_half = (list_node_t*)FLIP_BIT((uint64_t)curr_split_block, 12 + j);

            list_init(second_half);
            list_insert_after(&zone->orders[j].free_blocks_head, second_half);

            curr_split_block = first_half;
        }
        
        memset(curr_split_block, 0, PAGE_SIZE * ((size_t)1 << order));
        return (void*)curr_split_block;
    }

    return NULL;
}

static void zone_dealloc(allocator_zone_t *zone, uint64_t addr, int order)
{
    kassert_dbg(zone != NULL);
    kassert_dbg((addr & (~(PAGE_SIZE - 1))) == addr);
    kassert_dbg(order <= MAX_ORDER);

    uint64_t free_block_addr = addr;
    int free_order = order;
    for (; free_order < MAX_ORDER; free_order++)
    {
        uint64_t buddy_addr = FLIP_BIT(free_block_addr, 12 + free_order);

        size_t block_index = (free_block_addr - (uint64_t)zone->start_addr) / (((size_t)1 << free_order) * PAGE_SIZE);
        size_t buddy_index = block_index / 2;
        uint8_t *bitmap = zone->orders[free_order].bitmap;
        if (!GET_BIT(bitmap[buddy_index / 8], buddy_index % 8))
        {
            // Buddy is busy - can't coalesce
            bitmap[buddy_index / 8] = SET_BIT(bitmap[buddy_index / 8], buddy_index % 8);
            break;
        }
        
        // Coalesce
        bitmap[buddy_index / 8] = CLEAR_BIT(bitmap[buddy_index / 8], buddy_index % 8);        
        list_extract((list_node_t*)buddy_addr);

        if (buddy_addr < free_block_addr)
            free_block_addr = buddy_addr;
    }

    kassert_dbg((addr & (~(((size_t)1 << (12 + free_order)) - 1))) == free_block_addr);

    // Add final free block to the according list
    list_node_t *free_block = (list_node_t*)free_block_addr;
    list_init((list_node_t*)free_block);
    list_insert_after(&zone->orders[free_order].free_blocks_head, free_block);
}

static int pages2order(size_t pages)
{
    kassert_dbg(pages <= (1 << MAX_ORDER));
    int order = 0;
    while ((1 << order) < (int)pages)
        order++;

    return order;
}
