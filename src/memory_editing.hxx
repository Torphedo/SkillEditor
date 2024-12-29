#pragma once

#include "types.hxx"
#include "structs.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct {
    HANDLE h;
    uintptr_t gstorage_addr;
    gsdata* gstorage;
    u32 pid;
}pd_meta;

// Gets the process ID, attaches to it with read/write permissions, then retrieves a copy of gsdata
bool get_process(pd_meta* p);

// Refresh all process information as needed, including gsdata. Otherwise, nothing.
void update_process(pd_meta* p, bool force);

// If anything changed in gstorage since the last time this was called, all
// pages that changed are copied into the game process.
// @return Whether data changed and a copy was needed
bool flush_to_pd(pd_meta p);

bool handle_still_valid(HANDLE h);

bool is_running();

// Checks if 1 byte can be read from memory
bool can_read_memory(pd_meta p);

// Loads gsdata from memory into addressable structs.
bool load_skill_data(pd_meta p);
