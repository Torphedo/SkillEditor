#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <common/int.h>
#include <common/logging.h>
#include "pool.h"

bool pool_can_hold(pool_t pool, u32 size) {
    // Pool's... open?
    return pool.alloc_size > (pool.pos + size);
}

void* pool_getdata(pool_t pool, u32 handle) {
    return (void*)(pool.data + handle);
}

pool_t pool_open(u32 init_size) {
    return (pool_t) {
        .data = (uintptr_t)calloc(1, init_size),
        .alloc_size = init_size,
    };
}

void pool_close(pool_t* pool) {
    void* data = (void*)pool->data;
    *pool = (pool_t){0};

    // This order of operations makes sure there's never a dangling pointer.
    free((void*)data);
    // Pool's closed.
}

pool_handle pool_push(pool_t* pool, const void* data, u32 size, u32 padding_size) {
    // If there's no room, we need to realloc
    if (!pool_can_hold(*pool, size + padding_size)) {
        // The buffer is completely full & needs a new allocation.
        // Grow by 50%, rounded up to the next multiple of our element size.
        const u32 newsize = (pool->alloc_size + size + padding_size) * 1.5;
        assert(newsize > pool->alloc_size); // Sanity check to avoid memory corruption
        void* newbuf = calloc(1, newsize);
        if (newbuf == NULL) {
            LOG_MSG(error, "Couldn't expand 0x%X -> 0x%X [alloc failure]\n", pool->alloc_size, newsize);
            return POOL_INVALID_HANDLE;
        }

        // Copy data & update state
        memcpy(newbuf, (void*)pool->data, pool->alloc_size);
        free((void*)pool->data);
        pool->data = (uintptr_t)newbuf;
        pool->alloc_size = newsize;
    }

    // Copy the new data in & add it to the pool
    memcpy(pool_getdata(*pool, pool->pos), data, size);
    const pool_handle idx = pool->pos;
    pool->pos += size + padding_size;

    return idx;
}

void pool_drain(pool_t* pool) {
    memset((void*)pool->data, 0x00, pool->alloc_size);
    pool->pos = 0;
}

bool pool_empty(pool_t pool) {
    return pool.pos == 0 || pool.alloc_size == 0 || pool.data == 0;
}
