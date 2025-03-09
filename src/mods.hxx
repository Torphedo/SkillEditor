#pragma once
/// @file mods.hxx
/// @brief Functions for installing/saving mods.
/// This is the heart of the program, and the worst part of the codebase.
#include <cstddef>

#include <common/file.h>
#include "pool.h"

// Custom Skill Editor formats for skill packs

// Original format, skill data only
typedef struct {
    char name[32];
    u16 format_version; // 1
    u16 skill_count;
    u8 pad[12];
}pack_header1;
const u32 s = sizeof(pack_header1);

// Skill pack v2 format, has skill and text data
typedef struct {
    u16 name_length; // Should be calculated with strlen()
    u16 desc_length;
}pack2_text;

// =============================================================================
// Skill Pack v3 structures
typedef enum : u32 {
    // Stands for "Skill Pack v3"
    PACKV3_MAGIC = MAGIC('S', 'P', '3', 0x00)
}packv3_magic;

// All the data associated with each skill in a pack
typedef struct {
    skill_t skill; // The actual skill data

    // The index in gstorage this skill should overwrite
    u16 idx;

    // Skill packs have a string pool after all of the skill entries, which
    // stores all of the text related to the skills.
    // This is a similar idea, but different than the gstorage string pool in
    // vanilla files. Skill Editor copies the text into Phantom Dust's gstorage
    // string pool when loading the skill pack.

    // The offset in the skill pack's string pool where the skill's name is.
    u16 name_offset;
    // The offset in the skill pack's string pool where the skill's description is.
    u16 desc_offset;
}packv3_entry;

struct packv3_header {
    // Makes the file easy to identify as v3 in a hex editor
    packv3_magic magic = PACKV3_MAGIC;

    // Format version & skill count are @ 0x20 in other versions, so we add
    // padding to match that.
    u8 pad[0x20 - sizeof(packv3_magic)] = {0};

    u16 format_version = 3;
    u16 skill_count = 0;
    u8 pad2[12] = {0};

    packv3_header() = default;
    packv3_header(u16 skill_count) {
        this->skill_count = skill_count;
    }
};
// Header sizes should match
static_assert(sizeof(pack_header1) == sizeof(packv3_header));
// This might show up as an error in your editor, but it's valid and compiles.
// CLion complains about it and I'm not sure why...
static_assert(offsetof(pack_header1, format_version) == offsetof(packv3_header, format_version));

// Presents a file select dialog to the user, and saves the selected pack to a
// hidden internal pointer used by other functions.
// @return Whether there was any previously saved path.
bool skill_select();

/// Prompts the user for a skill file, then opens it and writes it to gsdata.
/// @return the ID of the loaded skill if successful, otherwise returns the ID that was given.
unsigned int install_skill(pd_meta p, unsigned int current_id);

void load_skill_v1_v2(FILE* skill_file, skill_t* skill_out, char** name_out, char** desc_out);

// Prompts the user for a filepath if they haven't entered one yet, then writes
// the specified skill (by ID) to disk and updates the version number and PD's gsdata.
void save_skill_to_file(pd_meta p, s16 id, bool write_text);
void save_skill_pack(const char* packname);

// Installs a skill pack into the game's memory.
void install_mod(pd_meta p);