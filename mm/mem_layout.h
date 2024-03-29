#ifndef MEM_LAYOUT_H
#define MEM_LAYOUT_H

#define PAGE_SIZE 4096

#define KERNEL_HIGHER_HALF_START 0xffff800000000000

#define KERNEL_SECTIONS_START    0xffffffff80000000
#define KERNEL_SECTIONS_SIZE     2 * (1ull << 30)

#define KERNEL_DIRECT_PHYS_MAPPING_START 0xffff888000000000
#define KERNEL_DIRECT_PHYS_MAPPING_SIZE  64 * (1ull << 40)

#endif
