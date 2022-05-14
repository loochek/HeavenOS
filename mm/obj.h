#ifndef OBJ_H
#define OBJ_H

#include "common.h"

typedef struct obj_alloc
{
    // Linked list of free objects
    void* next_free;
    size_t obj_size;
} obj_alloc_t;

// Creates allocator for specified type of objects
#define OBJ_ALLOC_DEFINE(var, type) obj_alloc_t var = { .next_free = NULL, .obj_size = sizeof(type) }

/**
 * Allocates single object and returns its virtual address.
 * 
 * \param alloc Object allocator
 */
void* object_alloc(obj_alloc_t* alloc);

/**
 * Frees object associated with given allocator
 * 
 * \param alloc Object allocator
 * \param obj Object
 */
void object_free(obj_alloc_t* alloc, void* obj);

#endif
