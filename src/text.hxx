#pragma once
#include <string>

#include "memory-editing.hxx"

typedef struct skill_text {
    std::string name;
    std::string desc;
}skill_text;

// Reads and prints all skill names and descriptions to the console.
skill_text load_skill_text(pd_meta p, unsigned int id);

// Writes skill text to memory
bool save_skill_text(pd_meta p, skill_text text, unsigned int id);

// Installs a skill pack into the game's memory.
void install_mod(pd_meta p);

// Toggles whether the game is currently frozen by Skill Editor
void toggle_game_pause(pd_meta p);
