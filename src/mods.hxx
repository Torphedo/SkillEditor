#pragma once
/// @file mods.hxx
/// @brief Functions for installing/saving mods.
/// This is the heart of the program, and the worst part of the codebase.

#include <common/file.h>

// Custom Skill Editor formats for skill packs

// Original format, skill data only
typedef struct {
    char name[32];
    u16 format_version; // 1
    u16 skill_count;
    u8 pad[12];
}pack_header1;

// Skill pack v2 format, has skill and text data
typedef struct {
    u16 name_length; // Should be calculated with strlen()
    u16 desc_length;
}pack2_text;

bool skill_select();

/// Prompts the user for a skill file, then opens it and writes it to gsdata.
/// @return the ID of the loaded skill if successful, otherwise returns the ID that was given.
unsigned int load_attack_skill(pd_meta p, unsigned int current_id);

// Prompts the user for a filepath if they haven't entered one yet, then writes
// the specified skill (by ID) to disk and updates the version number and PD's gsdata.
void save_skill_to_file(pd_meta p, unsigned int id, bool write_text);
void save_skill_pack(const char* packname);

// Installs a skill pack into the game's memory.
void install_mod(pd_meta p);