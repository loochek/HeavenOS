#ifndef EARLY_H
#define EARLY_H

/// Contains virtual addresses of early boot structures
typedef struct early_data
{
    void *multiboot_info;
} early_data_t;

#endif
