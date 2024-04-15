#pragma once
#include "types.h"
#include "structs.h"

typedef struct {
    void* h;
    uintptr_t gstorage_addr;
    gsdata* gstorage;
    u32 pid;
}pd_meta;

// Gets the process ID, attaches to it with read/write permissions, then retrieves a copy of gsdata
pd_meta get_process();

// Refresh all process information as needed, including gsdata. Otherwise, nothing.
void update_process(pd_meta* p, bool force);

bool still_running(void* handle);

// Checks if 1 byte can be read from memory
bool can_read_memory(pd_meta p);

// Loads gsdata from memory into addressable structs.
bool load_skill_data(pd_meta p);

// Reads and prints all skill names and descriptions to the console.
skill_text load_skill_text(pd_meta p, unsigned int id);

// Writes skill text to memory
bool save_skill_text(pd_meta p, skill_text text, unsigned int id);

// Write skill data and version number to memory.
bool write_gsdata_to_memory(pd_meta p);

// Installs a skill pack into the game's memory.
void install_mod(pd_meta p);

// Toggles whether the game is currently frozen by Skill Editor
void toggle_game_pause(pd_meta p);
