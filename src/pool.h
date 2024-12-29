#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include "types.hxx"

/// @brief An automatically expanding dynamic buffer
typedef struct {
    /// @brief Backing buffer
    ///
    /// We use a uintptr_t so we can have a typeless pointer that can't
    /// accidentally be dereferenced.
    uintptr_t data;
    /// Current backing buffer size
    u32 alloc_size;

    /// @brief Current position in backing buffer
    u32 pos;
}pool_t;

enum {
    // This handle value could only point to a single byte anyway, so it's fine
    // to not be able to use it.
    POOL_INVALID_HANDLE = UINT32_MAX,
};

typedef u32 pool_handle;

/// Create a pool.
/// @param init_size The allocation size in bytes. Please try to align to
///
/// @return A newly initialized dynamic pool.
/// @note This allocates memory!
/// @sa pool_close
pool_t pool_open(u32 init_size);

/// @brief Free pool data & fill all fields/buffer(s) with 0
///
/// @param pool Pool to destroy
/// @sa pool_open
void pool_close(pool_t* pool);

/// @brief Store data in the pool.
/// @param pool The pool to modify
/// @param data The data to store. Must be at least [size] bytes
/// @param size The number of bytes to be copied from [data]
/// @param padding_size The number of bytes to allocate in the pool. This is
/// the amount of memory that will actually be available for later use.
/// Useful for storing null-terminated strings.
/// @return A permanent, non-pointer handle to the data now stored in the pool.
/// You can get a pointer to the data via @ref pool_getdata(), but that may be
/// invalidated over time. The handle is the only safe way to refer to data in
/// the pool in the long term.
///
/// @note This causes a re-allocation if the pool is currently full.
/// @sa pool_open()
pool_handle pool_push(pool_t* pool, const void* data, u32 size, u32 reserved_size);

/// @brief Retrieve a temporary pointer to some data stored in the pool
///
/// @param The pool to look up the handle in
/// @param The pool handle obtained from @ref pool_push()
/// @return A non-permanent direct pointer to the data. This may be invalidated
/// when the pool is modified, so use it as sparingly as possible.
void* pool_getdata(pool_t pool, u32 handle);

/// Reset pool to initial state and fill buffer with 0. Does not free buffer.
void pool_drain(pool_t* pool);

/// @brief Whether the pool is empty
bool pool_empty(pool_t pool);

#ifdef __cplusplus
}
#endif