#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/// @file remote_pd.h
/// Functions for syncing state with Phantom Dust.

#include <stdbool.h>
#include <common/int.h>
#include "structs.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

enum {
    PD_VERSION_NUMBER = 140,
    REMOTE_REGION_MAX_PAGES = 512,
    REMOTE_REGION_MAX_SIZE = 4096 * REMOTE_REGION_MAX_PAGES,
};

// Local copy of a memory region in a remote process.
typedef struct {
    void* local_data;
    uintptr_t remote_addr;
    u32 size;
}remote_region;

// Minimum process access flags for using the remote region API
static const DWORD remote_region_access = PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | SYNCHRONIZE;

/// @brief Allocate a synced remote memory region
///
/// This API allows you to keep a local copy of a memory region in a remote
/// process' address space. The remote copy can then be overwritten with your
/// data using @ref flush_remote_region().
/// @param size Size of the synced region
/// @param remote_addr Pointer in the remote process' address space to sync with
/// @param h Process handle with at least [remote_region_access] access flags.
remote_region alloc_remote_region(u32 size, uintptr_t remote_addr, HANDLE h);

/// @brief Overwrite remote region with your local data
///
/// Only the memory pages that were changed since the last flush will be copied.
/// The CPU tracks these changes in hardware, and we query the information
/// through Windows API calls. Aside from having to copy entire pages at a time,
/// there is very little overhead.
/// @param reg Region to sync
/// @param h Process handle with at least [remote_region_access] access flags.
bool flush_remote_region(const remote_region* reg, HANDLE h);

/// @brief Free our local copy of the data and wipe the structure
void free_remote_region(remote_region* reg);

typedef struct {
    HANDLE h;
    uintptr_t gstorage_addr;
    remote_region gstorage;
    u32 pid;
} pd_meta;

// Gets the process ID, attaches to it with read/write permissions, then retrieves a copy of gsdata
bool get_process(pd_meta* p);

// Refresh all process information as needed, including gsdata. Otherwise, nothing.
void update_process(pd_meta* p, bool force);

// If anything changed in gstorage since the last time this was called, all
// pages that changed are copied into the game process.
// @return Whether data changed and a copy was needed
bool flush_to_pd(pd_meta p, bool use_vanilla_version);

bool handle_still_valid(HANDLE h);

bool is_running();

// Checks if 1 byte can be read from memory
bool can_read_memory(pd_meta p);

// Toggles whether the game is currently frozen by Skill Editor
void toggle_game_pause(pd_meta p);

#ifdef __cplusplus
}
#endif