#pragma once

bool skill_select();

// Prompts the user for a skill file, then opens it and writes it to gsdata. Returns the ID of the loaded skill if successful, otherwise returns the ID that was given.
unsigned int load_attack_skill(pd_meta p, unsigned int current_id);

// Prompts the user for a filepath if they haven't entered one yet, then writes the specified skill (by ID) to disk and updates the version number and PD's gsdata.
void save_skill_to_file(pd_meta p, unsigned int id, bool write_text);
void save_skill_pack(const char* packname);

// Installs a skill pack into the game's memory.
void install_mod(pd_meta p);
