#include "kernel/panic.h"
#include "obj.h"
#include "mm/frame_alloc.h"

void* object_alloc(obj_alloc_t* alloc)
{
    kassert_dbg(alloc != NULL);
    kassert(alloc->obj_size <= PAGE_SIZE);

    if (alloc->next_free == NULL)
    {
        // Request another page of memory
        void *page = frame_alloc();

        uint64_t curr = (uint64_t)page;
        uint64_t end = curr + PAGE_SIZE;

        uint64_t prev = 0;
        while (curr + alloc->obj_size <= end)
        {
            *(uint64_t*)curr = prev;
            prev = curr;

            curr += ROUNDUP(alloc->obj_size, CACHE_LINE_SIZE_BYTES);
        }

        alloc->next_free = (void*)prev;
    }

    void *obj = alloc->next_free;
    alloc->next_free = (void*)*(uint64_t*)alloc->next_free;
    return obj;
}

void object_free(obj_alloc_t* alloc, void* obj)
{
    kassert_dbg(alloc != NULL);
    kassert_dbg(obj != NULL);

    *(uint64_t*)obj = 0;
    alloc->next_free = obj;
}
