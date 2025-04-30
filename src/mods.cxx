#include <filesystem>
#include <cstdio>

#include "winAPI.hxx"
#include "remote_pd.hxx"
#include "text.hxx"
#include "mods.hxx"
#include "pool.h"

#include <common/crc32.h>
#include <common/file.h>
#include <common/logging.h>

// Used to track the most recently saved skill filepath
char* most_recent_filename = nullptr;

bool skill_select() {
    most_recent_filename = file_save_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.sp3;" }, L".sp3");
    return most_recent_filename != nullptr;
}

// Save functions don't need to deal with backwards compatibility and will
// change between versions

void save_skill_data(const char* path, skill_t skill, const char* name, const char* desc, u16 idx) {
    const packv3_header header = packv3_header(1);

    const packv3_entry entry = {
        skill,
        idx,
        0,
        (u16)(strlen(name) + 1),
    };

    // Save skill file
    FILE* f = fopen(path, "wb");
    if (f == nullptr) {
        printf("Failed to open file \"%s\"\n", path);
        return;
    }
    fwrite(&header, sizeof(header), 1, f);
    fwrite(&entry, sizeof(entry), 1, f);
    fwrite(name, strlen(name) + 1, 1, f);
    fwrite(desc, strlen(desc) + 1, 1, f);
    fclose(f);
}

// Writes the currently open skill to disk.
void save_skill_to_file(pd_meta p, s16 id, bool write_text) {
    // Check that we actually have data to write and a place to write it to
    if (id == 0) {
        printf("Skill ID was 0, skipping.\n");
        return;
    }

    if (most_recent_filename == nullptr) {
        most_recent_filename = file_select_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.sp3;" });
        if (most_recent_filename == nullptr) { // Must be checked again in case user cancels selection
            return;
        }
    }

    // Save the skill
    const u16 index = id - 1;
    const skill_t skill = p.gstorage->skill_array[index];
    // Use empty strings if told not to save text
    skill_text text = {"", ""};
    if (write_text) {
        text = get_skill_text(p, skill.SkillTextID);
    }
    save_skill_data(most_recent_filename, skill, text.name.data(), text.desc.data(), index);

    printf("Saved skill to %s\n", most_recent_filename);
}

/// @brief Determine if some data loaded from disk is a v3 skill pack
/// @param Pointer to the first byte of data [should have at least 4 readable bytes]
bool is_v3_pack(void* data) {
    return *((u32*)data) == PACKV3_MAGIC;
}
// Skill loading functions, which have to maintain backwards compatibility

void load_skill_v1_v2(FILE* skill_file, skill_t* skill_out, char** name_out, char** desc_out) {
    fread(skill_out, sizeof(*skill_out), 1, skill_file);

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
    p.gstorage->skill_array[skill.SkillID - 1] = skill;

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

void save_skill_pack() {
    // I believe we leak memory here, but it crashes if I free it for some reason...
    char* out_filepath = file_save_dialog(COMDLG_FILTERSPEC{ L"Skill Pack", L"*.sp3;" }, L".sp3");
    if (out_filepath == nullptr) {
        // No filepath?
        printf("No filepath given, pack save cancelled.\n");
        return;
    }
    FILE* skill_pack_out = fopen(out_filepath, "wb");
    if (skill_pack_out == nullptr) {
        printf("Couldn't open skill pack file \"%s\" for writing.\n", out_filepath);
        return;
    }

    const packv3_header header_out = packv3_header(MultiSelectCount);
    fwrite(&header_out, sizeof(header_out), 1, skill_pack_out);

    pool_t pool = pool_open(MultiSelectCount * 0x20); // Just an initial size
    // Text pool comes after header and skill entries
    for (u32 i = 0; i < MultiSelectCount; i++) {
        FILE* skill_file = fopen(multiselectpath[i].data(), "rb");
        if (skill_file == nullptr) {
            LOG_MSG(error, "Failed to open input file \"%s\"\n");
            continue;
        }

        // Read just enough data to find out if this is a v3 skill pack
        packv3_header header;
        fread(&header, sizeof(header), 1, skill_file);

        // V3 packs should have all their skills included
        if (is_v3_pack(&header.magic)) {
            packv3_entry* entries = (packv3_entry*)calloc(header.skill_count, sizeof(packv3_entry));

            if (entries == nullptr) {
                printf("Failed to allocate for skill data from \"%s\"", multiselectpath[i].data());
                free(entries);
                continue;
            }

            // Load all skill entries at once
            fread(entries, sizeof(*entries), header.skill_count, skill_file);

            // Text data takes up the remainder of the file:
            const u32 text_size = file_size(multiselectpath[i].data()) - sizeof(header) - sizeof(*entries) * header.skill_count;

            // Allocate space, then just copy the skill text directly into our pool.
            const pool_handle offset = pool_push(&pool, nullptr, 0, text_size);
            fread(pool_getdata(pool, offset), text_size, 1, skill_file);

            // Adjust text offsets in each entries to match their final location
            for (u32 i = 0; i < header.skill_count; i++) {
                entries[i].name_offset += offset;
                entries[i].desc_offset += offset;
            }

            // Native format, just copy the entries over
            fwrite(entries, sizeof(*entries), header.skill_count, skill_pack_out);

            // Cleanup
            free(entries);
        } else {
            // V1 or V2 file
            // Save the skill & text in the new format
            packv3_entry entry = {0};
            char* name = nullptr;
            char* desc = nullptr;
            if (!is_v3_pack((void*)&magic)) {
                // It's an old file, use backwards compatible loading
                fseek(skill_file, 0, SEEK_SET);
                load_skill_v1_v2(skill_file, &entry.skill, &name, &desc);
                entry.idx = entry.skill.SkillID - 1;
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
    printf("Saved skill pack to %s\n", out_filepath);
}

bool install_skill_pack_v1_v2(pd_meta p, FILE* skill_pack) {
    pack_header1 header = {0};
    fread(&header, sizeof(header), 1, skill_pack);

    skill_t* skills = (skill_t*)calloc(header.skill_count, sizeof(*skills));
    fread(skills, sizeof(*skills), header.skill_count, skill_pack);
    for (int i = 0; i < header.skill_count; i++) {
        p.gstorage->skill_array[(skills[i].SkillID - 1)] = skills[i]; // Write skills from pack into gsdata
    }
    pack2_text* text_meta = (pack2_text*) calloc(header.skill_count, sizeof(pack2_text));
    if (text_meta == nullptr) {
        // TODO: Just read in a loop so this can't happen
        printf("Failed to allocate for text metadata!\n");
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

bool install_skill_pack(pd_meta p, const char* path) {
    FILE* skill_pack = fopen(path, "rb");
    if (skill_pack == nullptr) {
        printf("Failed to open %s\n", path);
        return false;
    }

    packv3_header header;
    fread(&header, sizeof(header), 1, skill_pack);

    // Compatibility checks
    if (header.format_version <= 3 && header.magic != PACKV3_MAGIC) {
        // Older file, use backwards compatibility
        fseek(skill_pack, 0, SEEK_SET); // Reset pos
        if (header.format_version == 0) {
            // This is a single skill and not a pack, but it still has text.
            return install_skill_v1_v2(p, skill_pack);
        }
        return install_skill_pack_v1_v2(p, skill_pack);
    }
    else if (header.format_version > 3) {
        // Newer file version, we can't handle it.
        printf("Your skill pack \"%s\" was made for a newer version of Skill Editor, I don't know what to do with it. Cancelling.\n", path);
        return false;
    } else if (header.magic != PACKV3_MAGIC) {
        // Some invalid file
        printf("I don't recognize \"%s\" as a valid skill pack, cancelling.\n", path);
        return false;
    }

    // Load the pack's string pool. This makes things easy on our end and lets us load everything in one pass.
    const s32 pool_offset = sizeof(packv3_header) + (header.skill_count * sizeof(packv3_entry));
    // If the size ends up negative, MAX() will keep it positive
    const s64 pool_size = MAX(1, (s64)file_size(path) - pool_offset);
    pool_t pool = pool_open(pool_size);
    if (pool.data == 0) {
        // Alloc failure
        printf("Failed to allocate %llu bytes for \"%s\"'s string pool!\n", pool_size, path);
        return false;
    }

    // Jump to the text data and read it
    fseek(skill_pack, pool_offset, SEEK_SET);
    fread((void*)pool.data, pool_size, 1, skill_pack);
    pool.pos = pool_size;
    // Jump back to where we were
    fseek(skill_pack, sizeof(header), SEEK_SET);

    // Looks like this is a good pack file we can understand, time to install it
    for (u32 i = 0; i < header.skill_count; i++) {
        // Load the entry
        packv3_entry entry = {0};
        fread((void*)&entry, sizeof(entry), 1, skill_pack);

        // Copy the skill into gstorage
        p.gstorage->skill_array[entry.idx] = entry.skill;

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

void install_mod(pd_meta p) {
    if (!handle_still_valid(p.h)) {
        return;
    }

    for (int i = 0; i < MultiSelectCount; i++) {
        install_skill_pack(p, multiselectpath[i].c_str());
        printf("Installed skill pack %s.\n", multiselectpath[i].c_str());
    }
}