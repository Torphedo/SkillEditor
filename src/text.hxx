#pragma once
#include <string>

#include "memory_editing.hxx"

// All the text associated with a skill. These are views into the real string
// buffer, so copy them into a std::string if you need to change the size.
typedef struct skill_text {
    std::string_view name;
    std::string_view desc;
}skill_text;

// Finds the name/description of the provided skill text ID
skill_text load_skill_text(pd_meta p, unsigned int id);

// Writes skill text to memory
bool save_skill_text(pd_meta p, skill_text text, unsigned int id);

// Installs a skill pack into the game's memory.
void install_mod(pd_meta p);

// Toggles whether the game is currently frozen by Skill Editor
void toggle_game_pause(pd_meta p);