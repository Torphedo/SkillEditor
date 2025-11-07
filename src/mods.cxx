#include <stdio.h>
#include <cassert>

#include <nfd.h>

#include "remote_pd.h"
#include "text.h"
#include "mods.hxx"
#include "pool.h"

#include <common/crc32.h>
#include <common/file.h>
#include <common/logging.h>

bool skill_select(char** path_out) {
    const nfdu8filteritem_t filters[] = { { "Skill File", "sp4" } };
    nfdresult_t res = NFD_SaveDialogU8((nfdu8char_t**)path_out, filters, ARRAY_SIZE(filters), nullptr, nullptr);
    return res == NFD_OKAY;
}

/// @brief Determine if some data loaded from disk is a v4-compatible skill pack
/// @param Pointer to the first byte of data [should have at least 4 readable bytes]
bool is_v4_pack(void* data) {
    const u32 magic = *((u32*)data);
    return magic == PACKV4_MAGIC;
}

// Shift skill data back by 8 bytes to correct for broken offset in past versions
void skill_backshift(skill_t* skill) {
    u8* target = (u8*)skill;
    u8* source = target + 8;
    memmove(target, source, sizeof(*skill) - 8);

    u8* erase_target = target + (sizeof(*skill) - 8);
    memset(erase_target, 0, 8);
}

// Save functions don't need to deal with backwards compatibility and will
// change between versions

void save_skill_data(const char* path, skill_t skill, pd_meta p, u16 idx, bool write_text) {
    packv4_header header = packv4_header();
    header.skill_count = 1;
    header.anim_profile_count = 2;

    // Use empty strings if told not to save text
    skill_text text = {"", ""};
    if (write_text) {
        text = get_skill_text(p, skill.SkillTextID);
    }

    const packv4_entry entry = {
        skill,
        idx,
        0,
        (u16)(strlen(text.name) + 1),
    };

    const anim_profile* profiles = (anim_profile*)p.anim_profiles.local_data;
    const packv4_anim_entry anim_entries[] = {
        {
            profiles[skill.AnimProfileGround],
            skill.AnimProfileGround,
        },
        {
            profiles[skill.AnimProfileAir],
            skill.AnimProfileAir,
        },
    };

    // Save skill file
    FILE* f = fopen(path, "wb");
    if (!f) {
        LOG_MSG(error, "Failed to open file \"%s\"\n", path);
        return;
    }

    fwrite(&header, sizeof(header), 1, f);
    fwrite(&entry, sizeof(entry), 1, f);
    fwrite(anim_entries, sizeof(*anim_entries), ARRAY_SIZE(anim_entries), f);
    fwrite(text.name, strlen(text.name) + 1, 1, f);
    fwrite(text.desc, strlen(text.desc) + 1, 1, f);
    fclose(f);
}

// Writes the currently open skill to disk.
void save_skill_to_file(const char* path, pd_meta p, s16 id, bool write_text) {
    // Check that we actually have data to write and a place to write it to
    if (id == 0) {
        LOG_MSG(warning, "Skill ID was 0, skipping.\n");
        return;
    }

    if (path == nullptr) {
        LOG_MSG(warning, "No path to save to.\n");
        return;
    }

    // Save the skill
    const u16 index = id;
    const gsdata* gstorage = (gsdata*)p.gstorage.local_data;
    const skill_t skill = gstorage->skill_array[index];
    save_skill_data(path, skill, p, index, write_text);

    LOG_MSG(info, "Saved skill to %s\n", path);
}

void save_skill_pack(const char* out_path, const std::vector<std::string>& skillpaths) {
    FILE* skill_pack_out = fopen(out_path, "wb");
    if (!skill_pack_out) {
        LOG_MSG(error, "Couldn't open skill pack file \"%s\" for writing.\n", out_path);
        return;
    }

    packv4_header header_out = packv4_header();
    header_out.skill_count = skillpaths.size(),
    header_out.anim_profile_count = header_out.skill_count * 2; // 2 animations per skill
    fwrite(&header_out, sizeof(header_out), 1, skill_pack_out);

    pool_t pool = pool_open(skillpaths.size() * 0x20); // Just an initial size
    // Text pool comes after header and skill entries
    for (u32 i = 0; i < skillpaths.size(); i++) {
        const char* path = skillpaths[i].c_str();
        FILE* skill_file = fopen(path, "rb");
        if (skill_file == nullptr) {
            LOG_MSG(error, "Failed to open input file \"%s\"\n");
            continue;
        }

        // Read just enough data to find out if this is a v4 skill pack
        packv4_header header = packv4_header();
        fread(&header, sizeof(header), 1, skill_file);

        // V4 packs should have all their skills included
        if (is_v4_pack(&header.magic)) {
            packv4_entry* entries = (packv4_entry*)calloc(MAX(1, header.skill_count), sizeof(*entries));
            packv4_anim_entry* anim_entries = (packv4_anim_entry*)calloc(MAX(1, header.anim_profile_count), sizeof(*anim_entries));

            if (!entries || !anim_entries) {
                LOG_MSG(error, "Failed to allocate for skill data from \"%s\"", path);
                free(entries);
                free(anim_entries);
                continue;
            }

            // Load all entries at once
            fread(entries, sizeof(*entries), header.skill_count, skill_file);
            fread(anim_entries, sizeof(*anim_entries), header.anim_profile_count, skill_file);

            // Text data takes up the remainder of the file:
            const u32 text_size = file_size(path) - sizeof(header) - sizeof(*entries) * header.skill_count;

            // Allocate space, then just copy the skill text directly into our pool.
            const pool_handle offset = pool_push(&pool, nullptr, 0, text_size);
            fread(pool_getdata(pool, offset), text_size, 1, skill_file);

            // Adjust text offsets in each entries to match their final location
            for (u32 j = 0; j < header.skill_count; j++) {
                entries[j].name_offset += offset;
                entries[j].desc_offset += offset;
                if (header.format_version < 4) {
                    // Account for old broken skill offset
                    skill_backshift(&entries[j].skill);
                }
            }

            // Native format, just copy the entries over
            fwrite(entries, sizeof(*entries), header.skill_count, skill_pack_out);
            fwrite(anim_entries, sizeof(*anim_entries), header.anim_profile_count, skill_pack_out);

            // Cleanup
            free(entries);
            free(anim_entries);
        } else {
            // V1 or V2 file
            // Save the skill & text in the new format
            packv4_entry entry = {0};
            char* name = nullptr;
            char* desc = nullptr;
            if (!is_v4_pack((void*)&header.magic)) {
                // It's an old file, use backwards compatible loading
                fseek(skill_file, 0, SEEK_SET);
                load_skill_v1_v2(skill_file, &entry.skill, &name, &desc);
                entry.idx = entry.skill.SkillID;
                // Account for old broken skill offset
                skill_backshift(&entry.skill);
            }

            // Copy text data to the pool
            if (name != nullptr) {
                entry.name_offset = pool_push(&pool, name, strlen(name) + 1, 0);
            }
            if (desc != nullptr) {
                entry.desc_offset = pool_push(&pool, desc, strlen(desc) + 1, 0);
            }

            // Save entry
            fwrite(&entry, sizeof(entry), 1, skill_pack_out);

            free(name);
            free(desc);
        }
    }

    // Save all the text data at once
    fwrite((void*)pool.data, pool.pos, 1, skill_pack_out);
    pool_close(&pool);

    fclose(skill_pack_out);
    LOG_MSG(info, "Saved skill pack to %s\n", out_path);
}

// Skill loading functions, which have to maintain backwards compatibility

void load_skill_v1_v2(FILE* skill_file, skill_t* skill_out, char** name_out, char** desc_out) {
    fread(skill_out, sizeof(*skill_out), 1, skill_file);
    skill_backshift(skill_out);

    // Find filesize using stdio then reset pos to where it was
    const size_t pos = ftell(skill_file);
    fseek(skill_file, 0, SEEK_END);
    const size_t filesize = ftell(skill_file);
    fseek(skill_file, pos, SEEK_SET);

    // Load text if applicable
    if (filesize > 144) {
        // v2 skill files store name/desc length followed by the text
        pack2_text text_meta = {0};
        fread(&text_meta, sizeof(text_meta), 1, skill_file);

        // We over-allocate by a byte to make sure text is null-terminated
        char* name = (char*) calloc(1, text_meta.name_length + 1);
        char* desc = (char*) calloc(1, text_meta.desc_length + 1);
        fread(name, text_meta.name_length, 1, skill_file);
        fread(desc, text_meta.desc_length, 1, skill_file);

        // Give our pointers to the caller
        *name_out = name;
        *desc_out = desc;
    }
}

// Load a single v1 or v2 format skill.
// v1 is a flat 0x90 byte file as used in the game.
// v2 files can contain text data, but skills without text data will just be a
// regular v1 file.
unsigned int install_skill_v1_v2(pd_meta p, FILE* skill_file) {
    skill_t skill = {};
    char* name = nullptr;
    char* desc = nullptr;
    load_skill_v1_v2(skill_file, &skill, &name, &desc);

    // Load skill data
    gsdata* gstorage = (gsdata*)p.gstorage.local_data;
    skill_t* skills = gstorage->skill_array;
    skills[skill.SkillID] = skill;

    if (name != nullptr || desc != nullptr) {
        const s32 text_id = skill.SkillTextID;
        skill_text original_text = get_skill_text(p, text_id);

        // Use the new text if present
        if (name != nullptr) {
            original_text.name = name;
        }
        if (desc != nullptr) {
            original_text.desc = desc;
        }

        // This can handle changing lengths of the strings, so no length check is needed.
        save_skill_text(p, original_text, text_id);
    }

    free(name);
    free(desc);
    return skill.SkillID;
}

bool install_skill_pack_v1_v2(pd_meta p, FILE* skill_pack) {
    pack_header1 header = {0};
    fread(&header, sizeof(header), 1, skill_pack);

    skill_t* skills = (skill_t*)calloc(header.skill_count, sizeof(*skills));
    fread(skills, sizeof(*skills), header.skill_count, skill_pack);
    for (int i = 0; i < header.skill_count; i++) {
        gsdata* gstorage = (gsdata*)p.gstorage.local_data;
        skill_t* skill_array = gstorage->skill_array;

        skill_backshift(&skills[i]);
        skill_array[skills[i].SkillID] = skills[i]; // Write skills from pack into gsdata
    }
    pack2_text* text_meta = (pack2_text*) calloc(header.skill_count, sizeof(*text_meta));
    if (text_meta == nullptr) {
        // TODO: Just read in a loop so this can't happen
        LOG_MSG(error, "Failed to allocate for text metadata!\n");
        return false;
    }

    // Read text metadata
    fread(text_meta, sizeof(*text_meta), header.skill_count, skill_pack);

    if (header.format_version == 2) {
        for (uint16_t i = 0; i < header.skill_count; i++) {
            // Load the new text data
            char* name = (char*)calloc(1, text_meta[i].name_length + 1);
            char* desc = (char*)calloc(1, text_meta[i].desc_length + 1);
            fread(name, text_meta[i].name_length, 1, skill_pack);
            fread(desc, text_meta[i].desc_length, 1, skill_pack);

            // This can handle changing lengths of the strings, so no length check is needed.
            save_skill_text(p, {name, desc}, skills[i].SkillTextID);
            free(name);
            free(desc);
        }
    }

    free(text_meta);
    free(skills);

    return true;
}

bool install_skill_pack_v3(pd_meta p, FILE* skill_pack) {
    fseek(skill_pack, 0, SEEK_END);
    const s64 fileSize = ftell(skill_pack);
    fseek(skill_pack, 0, SEEK_SET);

    packv3_header header = packv3_header();
    fread(&header, sizeof(header), 1, skill_pack);

    // This should've already been verified
    assert(header.format_version == 3);
    assert(header.magic == PACKV3_MAGIC);

    // Load the pack's string pool. This makes things easy on our end and lets us load everything in one pass.
    const s32 pool_offset = sizeof(header) + (header.skill_count * sizeof(packv3_entry));
    // If the size ends up negative, MAX() will keep it positive
    const s64 pool_size = MAX(1, fileSize - pool_offset);
    pool_t pool = pool_open(pool_size);
    if (pool.data == 0) {
        // Alloc failure
        LOG_MSG(error, "Failed to allocate %llu bytes for string pool!\n", pool_size);
        return false;
    }

    // Jump to the text data and read it
    fseek(skill_pack, pool_offset, SEEK_SET);
    fread((void*)pool.data, pool_size, 1, skill_pack);
    pool.pos = pool_size;
    // Jump back to where we were
    fseek(skill_pack, sizeof(header), SEEK_SET);

    // Looks like this is a good pack file we can understand, time to install it

    // Skill pointer is adjusted for pre-v4 files
    gsdata* gstorage = (gsdata*)p.gstorage.local_data;
    skill_t* skills = gstorage->skill_array;
    for (u32 i = 0; i < header.skill_count; i++) {
        // Load the entry
        packv3_entry entry = {0};
        fread((void*)&entry, sizeof(entry), 1, skill_pack);

        // Copy the skill into gstorage
        skill_backshift(&entry.skill);
        skills[entry.idx] = entry.skill;

        if (entry.desc_offset - entry.name_offset <= 1) {
            // This indicates the name is empty, so we'll assume the skill is
            // meant to use the original text and leave it alone.
            // If someone really wants an empty name, a space will bypass this.
            continue;
        } else {
            const char* name = (char*)pool_getdata(pool, entry.name_offset);
            const char* desc = (char*)pool_getdata(pool, entry.desc_offset);
            save_skill_text(p, {name, desc}, entry.skill.SkillTextID);
        }
    }

    fclose(skill_pack);
    return true;
}

bool install_skill_pack(pd_meta p, const char* path) {
    FILE* skill_pack = fopen(path, "rb");
    if (skill_pack == nullptr) {
        LOG_MSG(error, "Failed to open %s\n", path);
        return false;
    }

    packv4_header header = packv4_header();
    fread(&header, sizeof(header), 1, skill_pack);

    const bool too_new = header.format_version > 4;

    if (too_new) {
        LOG_MSG(error, "Your skill pack \"%s\" was made for a newer version of Skill Editor, I don't know what to do with it. Cancelling.\n", path);
        return false;
    }

    // Compatibility checks
    if (header.format_version < 4) {
        fseek(skill_pack, 0, SEEK_SET); // Reset pos
        switch (header.format_version) {
            case 0:
                // This is 1 skill (not a pack), which may or may not have text
                return install_skill_v1_v2(p, skill_pack);
            case 1:
                // fallthrough
            case 2:
                // Old skill pack format
                return install_skill_pack_v1_v2(p, skill_pack);
            case 3:
                return install_skill_pack_v3(p, skill_pack);
        }
    }

    const bool bad_magic = !is_v4_pack(&header);
    if (bad_magic) {
        // Some invalid file
        LOG_MSG(error, "I don't recognize \"%s\" as a valid skill pack, cancelling.\n", path);
        return false;
    }

    // Load the pack's string pool. This makes things easy on our end and lets us load everything in one pass.
    const s32 anim_offset = sizeof(header) + (header.skill_count * sizeof(*header.skills));
    const s32 pool_offset = anim_offset + (header.anim_profile_count * sizeof(packv4_anim_entry));
    // If the size ends up negative, MAX() will keep it positive
    const s64 pool_size = MAX(1, (s64)file_size(path) - pool_offset);
    pool_t pool = pool_open(pool_size);
    if (pool.data == 0) {
        // Alloc failure
        LOG_MSG(error, "Failed to allocate %llu bytes for \"%s\"'s string pool!\n", pool_size, path);
        return false;
    }

    // Jump to the text data and read it
    fseek(skill_pack, pool_offset, SEEK_SET);
    fread((void*)pool.data, pool_size, 1, skill_pack);
    pool.pos = pool_size;
    // Jump back to where we were
    fseek(skill_pack, sizeof(header), SEEK_SET);

    gsdata* gstorage = (gsdata*)p.gstorage.local_data;
    skill_t* skills = gstorage->skill_array;
    for (u32 i = 0; i < header.skill_count; i++) {
        // Load the entry
        packv4_entry entry = {0};
        fread((void*)&entry, sizeof(entry), 1, skill_pack);

        // Copy the skill into gstorage
        skills[entry.idx] = entry.skill;

        if (entry.desc_offset - entry.name_offset <= 1) {
            // This indicates the name is empty, so we'll assume the skill is
            // meant to use the original text and leave it alone.
            // If someone really wants an empty name, a space will bypass this.
            continue;
        } else {
            const char* name = (char*)pool_getdata(pool, entry.name_offset);
            const char* desc = (char*)pool_getdata(pool, entry.desc_offset);
            LOG_MSG(debug, "Loading '%s'\n", name);
            save_skill_text(p, {name, desc}, entry.skill.SkillTextID);
        }
    }

    // Load profiles if needed
    anim_profile* profiles = (anim_profile*)p.anim_profiles.local_data;
    for (u32 i = 0; i < header.anim_profile_count; i++) {
        packv4_anim_entry entry = {0};
        fread((void*)&entry, sizeof(entry), 1, skill_pack);
        profiles[entry.idx] = entry.anim;
    }

    fclose(skill_pack);
    return true;
}

void install_mod(pd_meta p, const std::string* paths, u32 path_num) {
    for (int i = 0; i < path_num; i++) {
        install_skill_pack(p, paths[i].c_str());
        LOG_MSG(info, "Installed skill pack %s.\n", paths[i].c_str());
    }
}