#pragma once
#include "structs.h"

// Get the process ID, attach to it with read/write permissions, and locate gsdata in its memory.
bool get_process();

// Loads skill data and version number from memory into addressable structs.
bool load_skill_data();

// Reads and prints all skill names and descriptions to the console.
bool load_skill_text();

// Write skill data and version number to memory.
bool write_gsdata_to_memory();

// Installs a skill pack into the game's memory.
void install_mod();

// Freezes the game's execution.
void pause_game();

// Unfreezes the game.
void resume_game();
