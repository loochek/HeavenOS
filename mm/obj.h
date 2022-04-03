#ifndef OBJ_H
#define OBJ_H

#include "common.h"

typedef struct obj_alloc
{
    // Linked list of free objects
    void* next_free;
    size_t obj_size;
} obj_alloc_t;

#define OBJ_ALLOC_DEFINE(var, type) obj_alloc_t var = { .next_free = NULL, .obj_size = sizeof(type) }

// object_alloc allocates single object and returns its virtual address.
void* object_alloc(obj_alloc_t* alloc);

// object_free frees obj associated with given allocator.
void object_free(obj_alloc_t* alloc, void* obj);

#endif
