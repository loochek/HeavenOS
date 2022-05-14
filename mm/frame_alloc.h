#ifndef FRAME_ALLOC_H
#define FRAME_ALLOC_H

#include "common.h"

/**
 * Initializes frame allocator. Must be called after direct physical memory mapping is created.
 */
void frame_alloc_init();

/**
 * Allocates continuous physical memory region.
 * 
 * \param size Amount of frames 
 */
void* frames_alloc(size_t size);

/**
 * Allocates single physical frame.
 */
void* frame_alloc();

/**
 * Frees specified amount of physical frames at specified base address
 * 
 * \param addr Base address
 * \param size Amount of frames
 */
void frames_free(void* addr, size_t size);

/**
 * Frees single physical frame at specified address
 * 
 * \param addr Base address
 */
void frame_free(void* addr);

#endif
