#pragma once

#include "types.hxx"
#include "structs.h"

typedef struct {
    void* h;
    uintptr_t gstorage_addr;
    gsdata* gstorage;
    u32 pid;
}pd_meta;

// Gets the process ID, attaches to it with read/write permissions, then retrieves a copy of gsdata
bool get_process(pd_meta* p);

// Refresh all process information as needed, including gsdata. Otherwise, nothing.
void update_process(pd_meta* p, bool force);

void flush_to_pd(pd_meta p);

bool handle_still_valid(void* handle);

bool is_running();

// Checks if 1 byte can be read from memory
bool can_read_memory(pd_meta p);

// Loads gsdata from memory into addressable structs.
bool load_skill_data(pd_meta p);
