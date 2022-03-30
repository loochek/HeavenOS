#include "common.h"
#include "kernel/multiboot.h"
#include "kernel/panic.h"
#include "mm/paging.h"
#include "mm/frame_alloc.h"

#define MAX_ORDER 10
#define MAX_ZONE_COUNT 10

typedef struct list_node
{
    struct list_node *next;
} __attribute__((aligned(PAGE_SIZE))) list_node_t;

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

static bool list_empty(list_node_t *head)
{
    kassert_dbg(head != NULL);
    return head->next == head;
}

static void list_init(list_node_t *head)
{
    kassert_dbg(head != NULL);
    head->next = head;
}

static void list_insert(list_node_t *head, list_node_t *new)
{
    kassert_dbg(head != NULL);
    kassert_dbg(new != NULL);
    new->next = head->next;
    head->next = new;
}

static list_node_t *list_pop(list_node_t *head)
{
    kassert_dbg(head != NULL);
    if (list_empty(head))
        return NULL;

    list_node_t *ret = head->next;
    head->next = ret->next;

    list_init(ret);
    return ret;
}

static void zone_add(uint64_t addr, size_t pages_count)
{
    kassert((addr & (PAGE_SIZE - 1)) == addr);
    kassert(pages_count > (1 << MAX_ORDER));
    kassert(zones_count < MAX_ZONE_COUNT);

    allocator_zone_t *zone = &allocator_zones[zones_count++];
    zone->start_addr = (void*)PHYS_TO_VIRT(addr);

    // 1. Determine max possible bitmap size and reserve space for it

    // In bytes
    size_t required_bitmap_size = 0;
    for (int i = 0; i <= MAX_ORDER; i++)
        required_bitmap_size += DIV_ROUNDUP(pages_count / (1 << i) / 2, 8);

    // In pages
    size_t bitmap_size = DIV_ROUNDUP(required_bitmap_size, PAGE_SIZE);
    uint64_t bitmap_ptr = zone->start_addr;
    zone->start_addr += PAGE_SIZE * bitmap_size;
    pages_count -= bitmap_size;

    // 2. Align remaining part of the zone by the max allocation size
    // (Wasting up to 2 * (1 << MAX_ORDER) pages of out precious memory!)
    zone->start_addr = ROUNDUP  (zone->start_addr,  PAGE_SIZE * (1 << MAX_ORDER));
    zone->end_addr   = ROUNDDOWN(zone->start_addr + pages_count * PAGE_SIZE, PAGE_SIZE * (1 << MAX_ORDER));

    zone->pages_count = ((uint64_t)zone->end_addr - (uint64_t)zone->start_addr) / PAGE_SIZE;

    // 2. Fill bitmaps pointers

    for (int i = 0; i < MAX_ORDER; i++)
    {
        size_t order_bitmap_size = DIV_ROUNDUP(zone->pages_count / (1 << MAX_ORDER) / 2, 8);
        zone->orders[i].bitmap = (uint8_t*)bitmap_ptr;
        bitmap_ptr += order_bitmap_size;
        kassert(bitmap_ptr < bitmap_ptr + bitmap_size * PAGE_SIZE);

        list_init(&zone->orders[i].free_blocks_head);
    }

    zone->orders[MAX_ORDER].bitmap = NULL;
    list_init(&zone->orders[MAX_ORDER].free_blocks_head);

    for (int i = 0; i < zone->pages_count / (1 << MAX_ORDER); i++)
    {
        list_node_t *block_ptr = zone->start_addr + i * (1 << MAX_ORDER) * PAGE_SIZE;
        list_init(block_ptr);
        list_insert(&zone->orders[MAX_ORDER].free_blocks_head, block_ptr);
    }
}

static void *zone_alloc(allocator_zone_t *zone, int order)
{
    assert_dbg(zone != NULL);
    assert(order <= MAX_ORDER);

    list_node_t *order_blocks_head = &zone->orders[order].free_blocks_head;
    if (!list_empty(order_blocks_head))
    {
        // Perfect fit
        list_node_t *block = list_pop(order_blocks_head);
        memset(block, 0, PAGE_SIZE * (1 << order));

        return (void*)block;
    }

    // Check for larger blocks
    for (int i = order; i <= MAX_ORDER; i++)
    {
        list_node_t *order_blocks_head = &zone->orders[i].free_blocks_head;
        if (list_empty(order_blocks_head))
            continue;

        // Found larger block, split it

        list_node_t *curr_split_block = list_pop(order_blocks_head);
        for (int j = i; j > order; j--)
        {
            list_node_t *first_half  = curr_split_block;
            list_node_t *second_half = (list_node_t*)FLIP_BIT((uint64_t)curr_split_block, 12 + j + 1);

            list_init(second_half);
            list_insert(&zone->orders[j].free_blocks_head, second_half);

            curr_split_block = first_half;
        }

        int block_index = ((uint64_t)curr_split_block - (uint64_t)zone->start_addr) / ((1 << order) * PAGE_SIZE);
        uint8_t *bitmap = zone->orders[order].bitmap;
        bitmap[block_index / 8] = FLIP_BIT(bitmap[block_index / 8], block_index % 8);
        
        memset(curr_split_block, 0, PAGE_SIZE * (1 << order));
        return (void*)curr_split_block;
    }

    return NULL;
}

static void zone_dealloc(allocator_zone_t *zone, void* block, int order)
{
    // TODO:
}

// static bool intersects(frame_t* frame, mem_region_t area)
// {
//     return area.start <= (void*)frame && (void*)frame < area.end;
// }

// static size_t preserved_areas_size = 0;
// static mem_region_t preserved_areas[100];

// // mark_preserved_area marks memory area as allocated, all its frames are not touched by frame allocator.
// static void mark_preserved_area(mem_region_t area)
// {
//     area.end = (void*)ALIGN_UP(area.end, PAGE_SIZE);
//     preserved_areas[preserved_areas_size++] = area;
// }

// // Those constants are defined by linker script.
// extern int _phys_start_kernel_sections;
// extern int _phys_end_kernel_sections;

// // mark_preserved_areas fills pre-allocated regions: early & kernel sections, multiboot info, etc
// void mark_preserved_areas() {
//     mark_preserved_area((mem_region_t){ .start = PHYS_TO_VIRT(&_phys_start_kernel_sections), .end = PHYS_TO_VIRT(&_phys_end_kernel_sections) });
//     mark_preserved_area(multiboot_mem_region());
// }

// // intersects_with_kernel_sections checks if a frame at the given virtual address intersects with kernel sections.
// static bool is_allocated(frame_t* frame)
// {
//     for (size_t i = 0; i < preserved_areas_size; i++)
//     {
//         if (intersects(frame, preserved_areas[i]))
//         {
//             return true;
//         }
//     }
//     return false;
// }

// static frame_t* freelist_head;

// // allocated_memory_region adds a region of RAM to the frame allocator.
// static size_t frame_alloc_add_area(frame_t* base, size_t sz)
// {
//     size_t pgcnt = 0;
//     frame_t* prev_free = NULL;
//     frame_t* first_free = NULL;
//     for (size_t i = 0; i < sz - 1; i++) {
//         frame_t* curr = &base[i];
//         if (is_allocated(curr)) {
//             continue;
//         }
//         if (first_free == NULL) {
//             prev_free = curr;
//             first_free = curr;
//             pgcnt++;
//             continue;
//         }
//         prev_free->next = curr;
//         prev_free = curr;
//         pgcnt++;
//     }
//     prev_free->next = freelist_head;
//     freelist_head = first_free;
//     return pgcnt;
// }

// // frame_alloc_add_areas obtains memory areas of usable RAM from Multiboot2 and adds them to the frame allocator.
// static void frame_alloc_add_areas()
// {
//     mb_memmap_iter_t mmap_it;
//     mb_memmap_iter_init(&mmap_it);
//     mb_memmap_entry_t *mmap_entry;
//     size_t pgcnt = 0;
//     while ((mmap_entry = mb_memmap_iter_next(&mmap_it)) != NULL)
//     {
//         if (mmap_entry->type != MB_MEMMAP_TYPE_RAM)
//         {
//             continue;
//         }

//         pgcnt += frame_alloc_add_area(PHYS_TO_VIRT(mmap_entry->base_addr), mmap_entry->length / PAGE_SIZE);
//     }

//     printk("initialized page_alloc with %d pages\n", pgcnt);
// }

// void frame_alloc_init()
// {
//     mark_preserved_areas();
//     frame_alloc_add_areas();
// }

// void* frames_alloc(size_t n) 
// {
//     if (freelist_head == NULL)
//     {
//         return NULL;
//     }

//     BUG_ON(n != 1);
//     frame_t* frame = freelist_head;
//     freelist_head = frame->next;
//     frame->next = NULL;
//     return frame;
// }

// void frames_free(void* addr, size_t n)
// {
//     BUG_ON(addr == NULL);
//     BUG_ON(n != 1);

//     frame_t* frame = addr;
//     frame->next = freelist_head;
//     freelist_head = frame;
// }

// void* frame_alloc()
// {
//     return frames_alloc(1);
// }

// void frame_free(void* addr)
// {
//     return frames_free(addr, 1);
// }
