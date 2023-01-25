#pragma once

void skill_select();

// Prompts the user for a skill file, then opens it and writes it to gsdata. Returns the ID of the loaded skill if successful, otherwise returns the ID that was given.
unsigned int load_attack_skill(unsigned int current_id);

// Prompts the user for a filepath if they haven't entered one yet, then writes the specified skill (by ID) to disk and updates the version number and PD's gsdata.
void save_skill_to_file(unsigned int id, bool write_text);
void save_skill_pack(const char* packname);
