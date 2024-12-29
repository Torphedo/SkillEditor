#include <filesystem>
#include <cstdio>

#include "winAPI.hxx"
#include "remote_pd.hxx"
#include "text.hxx"
#include "mods.hxx"
#include "pool.h"

#include <common/crc32.h>
#include <common/file.h>

// Used to track the most recently saved skill filepath
char* most_recent_filename = nullptr;

bool skill_select() {
    most_recent_filename = file_save_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" }, L".skill");
    return most_recent_filename != nullptr;
}

// Save functions don't need to deal with backwards compatibility and will
// change between versions

// Writes the currently open skill to disk.
void save_skill_to_file(pd_meta p, unsigned int id, bool write_text) {
    // Check that we actually have data to write and a place to write it to
    if (p.gstorage->skill_array[id - 1].SkillID != 0) {
        if (most_recent_filename == nullptr) {
            most_recent_filename = file_select_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" });
        }
        if (most_recent_filename != nullptr) { // Must be checked again in case user cancels selection
            FILE* skill_out = fopen(most_recent_filename, "wb");
            fwrite(&p.gstorage->skill_array[id - 1], sizeof(skill_t), 1, skill_out);
            if (write_text) {
                auto skill_text = load_skill_text(p, p.gstorage->skill_array[id - 1].SkillTextID + 1);
                pack2_text text_meta = {
                        .name_length = (uint16_t) (skill_text.name.length() + 1),
                        .desc_length = (uint16_t) (skill_text.desc.length() + 1)
                };
                fwrite(&text_meta, sizeof(text_meta), 1, skill_out);
                fwrite(skill_text.name.data(), skill_text.name.length() + 1, 1, skill_out);
                fwrite(skill_text.desc.data(), skill_text.desc.length() + 1, 1, skill_out);
            }
            fclose(skill_out);

            printf("Saved skill to %s\n", most_recent_filename);
        }
    }
    else {
        printf("Invalid skill ID.\n");
    }
}

// Load a skill file, adding its text to the given text pool if needed.
// @return The loaded skill data.
skill_t packv3_load_entry(const char* filename, pool_t* pool) {
    skill_t skill = {0};
    char* name = "";
    char* desc = "";
    FILE* f = fopen(filename, "rb");
    if (f == nullptr) {
        printf("Failed to open skill file \"%s\"\n", filename);
        return skill;
    }

    // Load the data & text in a backwards-compatible way
    load_skill_data_v1_v2(f, &skill, &name, &desc);

    // Add to pool and free buffers
    pool_push(pool, name, strlen(name), 0);
    pool_push(pool, desc, strlen(desc), 0);
    free(name);
    free(desc);

    fclose(f);
    return skill;
}

void save_skill_pack(const char* packname) {
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

    packv3_header header = {PACKV3_MAGIC};
    header.skill_count = MultiSelectCount;
    header.format_version = 3;

    pool_t pool = pool_open(MultiSelectCount * 0x20);
    for (u32 i = 0; i < MultiSelectCount; i++) {

    }

    fclose(skill_pack_out);
    printf("Saved skill pack to %s\n", out_filepath);
}

// Skill loading functions, which have to maintain backwards compatibility

/// @brief Determine if some data loaded from disk is a v3 skill pack
/// @param Pointer to the first byte of data [should have at least 4 readable bytes]
bool is_v3_pack(void* data) {
    return *((u32*)data) == PACKV3_MAGIC;
}

void load_skill_data_v1_v2(FILE* skill_file, skill_t* skill_out, char** name_out, char** desc_out) {
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
unsigned int load_skill_v1_v2(pd_meta p, FILE* skill_file) {
    skill_t skill = {};
    char* name = nullptr;
    char* desc = nullptr;
    load_skill_data_v1_v2(skill_file, &skill, &name, &desc);

    // Load skill data
    p.gstorage->skill_array[skill.SkillID - 1] = skill;

    if (name != nullptr || desc != nullptr) {
        const s32 text_id = skill.SkillTextID + 1;
        skill_text original_text = load_skill_text(p, text_id);

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

unsigned int load_skill(pd_meta p, unsigned int current_id) {
    most_recent_filename = file_select_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" });
    if (most_recent_filename == nullptr) {
        return current_id;
    }


    FILE* skill_file = fopen(most_recent_filename, "rb");
    if (skill_file == nullptr) {
        printf("Unable to open skill file \"%s\"\n", most_recent_filename);
        return current_id;
    }

    // Read just enough data to find out if this is a v3 skill pack
    u32 magic = 0;
    fread(&magic, sizeof(magic), 1, skill_file);
    fseek(skill_file, 0, SEEK_SET);

    if (!is_v3_pack((void*)&magic)) {
        // It's an old file, use backwards compatible loading
        return load_skill_v1_v2(p, skill_file);
    }
    printf("v3 skill loading is unimplemented, sorry.\n");

    fclose(skill_file)
    return current_id;
}

bool install_skill_pack_v1_v2(pd_meta p, FILE* skill_pack) {
    pack_header1 header = {0};
    fread(&header, sizeof(header), 1, skill_pack);

    skill_t* skills = (skill_t*)calloc(1, header.skill_count);
    fread(skills, sizeof(*skills), header.skill_count, skill_pack);
    for (int i = 0; i < header.skill_count; i++) {
        p.gstorage->skill_array[(skills[i].SkillID - 1)] = skills[i]; // Write skills from pack into gsdata
    }
    pack2_text* text_meta = (pack2_text*) calloc(1, sizeof(pack2_text) * header.skill_count);
    if (text_meta == nullptr) {
        // TODO: Just read in a loop so this can't happen
        printf("Failed to allocate for text metadata!\n");
        return false;
    }

    // Read text metadata
    fread(text_meta, sizeof(pack2_text), header.skill_count, skill_pack);

    if (header.format_version == 2) {
        for (uint16_t i = 0; i < header.skill_count; i++) {
            const s32 text_id = skills[i].SkillTextID + 1;

            // Load the new text data
            char* name = (char*)calloc(1, text_meta[i].name_length);
            char* desc = (char*)calloc(1, text_meta[i].desc_length);
            fread(name, text_meta[i].name_length, 1, skill_pack);
            fread(desc, text_meta[i].desc_length, 1, skill_pack);

            // This can handle changing lengths of the strings, so no length check is needed.
            save_skill_text(p, {name, desc}, text_id);
            free(name);
            free(desc);
        }
    }

    free(text_meta);
    free(skills);

    return true;
}

bool install_skill_pack(pd_meta p, const char* path) {
    packv3_header header = {};
    FILE* skill_pack = fopen(path, "rb");
    if (skill_pack == nullptr) {
        printf("Failed to open %s\n", path);
        return false;
    }
    fread(&header, sizeof(header), 1, skill_pack);

    // Compatibility checks
    if (header.format_version <= 3 && header.magic != PACKV3_MAGIC) {
        // Older file, use backwards compatibility
        fseek(skill_pack, 0, SEEK_SET); // Reset pos
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
    const u64 pool_offset = sizeof(packv3_header) + (header.skill_count * sizeof(packv3_entry));
    // If the size ends up negative, MAX() will keep it positive
    const s64 pool_size = MAX(1, (s64)file_size(path) - pool_offset);
    pool_t pool = pool_open(pool_size);
    if (pool.data == 0) {
        // Alloc failure
        printf("Failed to allocate %llu bytes for \"%s\"'s string pool!\n", pool_size, path);
        return false;
    }

    // Read the text data
    fread((void*)pool.data, pool_size, 1, skill_pack);
    pool.pos = pool_size;

    // Looks like this is a good pack file we can understand, time to install it
    for (u32 i = 0; i < header.skill_count; i++) {
        // Load the entry
        packv3_entry entry = {0};
        fread((void*)&entry, sizeof(entry), 1, skill_pack);

        // Copy the skill into gstorage
        p.gstorage->skill_array[entry.idx] = entry.skill;

        const char* name = (char*)pool_getdata(pool, entry.name_offset);
        const char* desc = (char*)pool_getdata(pool, entry.desc_offset);
        save_skill_text(p, {name, desc}, entry.skill.SkillTextID);
    }

    fclose(skill_pack);
    return true;
}

void install_mod(pd_meta p) {
    if (!handle_still_valid(p.h)) {
        return;
    }

    for (int n = 0; n < MultiSelectCount; n++) {
        install_skill_pack(p, multiselectpath[n].c_str());
    }
}