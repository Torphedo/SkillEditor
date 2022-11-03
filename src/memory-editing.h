#pragma once
#include "structs.h"

extern std::fstream AtkSkillFile;
extern GSDataHeader gsdataheader;
extern atkskill skillarray[751];
extern AttackSkill AtkSkill; // Attack skill struct

// Find a process' ID, given its executable name
DWORD get_pid_by_name(LPCTSTR ProcessName);

// Get the process ID, attach to it with read/write permissions, and locate gsdata in its memory.
bool get_process();

// Loads skill data and version number from memory into addressable structs.
int load_gsdata_from_memory();

// Write skill data and version number to memory.
int write_gsdata_to_memory();

// Installs a skill pack into the game's memory.
void install_mod();

// Freezes the game's execution.
void pause_game();
// Unfreezes the game.
void resume_game();
