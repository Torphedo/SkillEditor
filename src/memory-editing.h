#pragma once
#include "structs.h"

// Gets the process ID, attaches to it with read/write permissions, then updates our copy of gsdata if it's missing or the game was rebooted
bool get_process();

// Checks if the game is running.
bool is_running();

// Returns the current process ID.
DWORD process_id();

// Checks if 1 byte can be read from memory
bool can_read_memory();

// Returns the process handle by value
bool have_process_handle();

// Loads gsdata from memory into addressable structs.
bool load_skill_data();

// Reads and prints all skill names and descriptions to the console.
skill_text load_skill_text(unsigned int id);

// Writes skill text to memory
bool save_skill_text(skill_text text, unsigned int id);

// Write skill data and version number to memory.
bool write_gsdata_to_memory();

// Installs a skill pack into the game's memory.
void install_mod();

// Toggles whether the game is currently frozen by Skill Editor
void toggle_game_pause();
