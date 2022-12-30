#pragma once
#include "structs.h"

// Get the process ID, attach to it with read/write permissions, and locate gsdata in its memory.
bool get_process();

// Checks if the game is running.
bool is_running();

// Returns the current process ID.
DWORD process_id();

// Checks if 1 byte can be read from memory
bool can_read_memory();

// Returns the process handle by value
bool have_process_handle();

// Loads skill data and version number from memory into addressable structs.
bool load_skill_data();

// Reads and prints all skill names and descriptions to the console.
skill_text load_skill_text(unsigned int id);

// Write skill data and version number to memory.
bool write_gsdata_to_memory();

// Installs a skill pack into the game's memory.
void install_mod();

// Freezes the game's execution.
void pause_game();

// Unfreezes the game.
void resume_game();
