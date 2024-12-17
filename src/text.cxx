#include <cstring>
#include <windows.h>

extern "C" {
#include "crc_32.h"
}
#include "memory_editing.hxx"

#include "types.hxx"
#include "structs.h"
#include "text.hxx"
#include "winAPI.hxx"

skill_text load_skill_text(pd_meta p, unsigned int id) {
    if (p.h == INVALID_HANDLE_VALUE || p.h == NULL) {
        return {"", ""};
    }

    // Read section header to check the number of strings
    const text_header* header = &p.gstorage->textHeader;

    if (id > header->offset_count - 1) {
        return {"", ""};
    }

    const text_ptrs* string_offset = &p.gstorage->textPtrs[id - 1];

    const char* name = ((char*)string_offset) + string_offset->name;
    const char* desc = ((char*)string_offset) + string_offset->desc;

    skill_text output = { name, desc };

    return output;
}

void shift_textbuf(pd_meta p, char* offender, u32 new_len, u32 offender_id, bool is_name) {
    const s32 diff = new_len - (strlen(offender) + 1);
    if (diff <= 0 || offender < (char*)p.gstorage) {
        return;
    }

    const u64 remaining_space = sizeof(p.gstorage->textbuf) - (u64)(offender - p.gstorage->textbuf);
    char* target = offender + strlen(offender) + 1; // Start of the data to move
    memmove(target + diff, target, remaining_space);
    memset(target, 0, diff); // Clear the new space

    for (int i = offender_id + 1; i < ARRAYSIZE(p.gstorage->textPtrs); i++) {
        text_ptrs* offsets = &p.gstorage->textPtrs[i];
        offsets->name += diff;
        offsets->desc += diff;
    }

    // If we update the position of a name, the description's offset needs to update too.
    if (is_name) {
        text_ptrs* offsets = &p.gstorage->textPtrs[offender_id];
        offsets->desc += diff;
    }
}

bool save_skill_text(pd_meta p, skill_text text, unsigned int id) {
    if (p.h == INVALID_HANDLE_VALUE) {
        return false;
    }

    const text_header* header = &p.gstorage->textHeader;
    if (id > header->offset_count - 1) {
        return false;
    }

    // Get offset data
    const text_ptrs* string_offset = &p.gstorage->textPtrs[id - 1];

    // Shorthands for the new text
    const char* new_name = text.name.data();
    const char* new_desc = text.desc.data();
    const u32 name_size = strlen(new_name) + 1;
    const u32 desc_size = strlen(new_desc) + 1;

    char* name = ((char*)&p.gstorage->textPtrs) + string_offset->name;
    shift_textbuf(p, name, name_size, id - 1, true);

    // IMPORTANT: This must come after the above shift_textbuf call. If the name
    // string changes size, the description offset will be different.
    char* desc = ((char*)&p.gstorage->textPtrs) + string_offset->desc;
    shift_textbuf(p, desc, desc_size, id - 1, false);

    // Apply text changes to the string pool
    memcpy((void*)name, new_name, name_size);
    memcpy((void*)desc, new_desc, desc_size);

    return true;
}

void install_mod(pd_meta p) {
    if (!handle_still_valid(p.h)) {
        return;
    }
    for (int n = 0; n < MultiSelectCount; n++) {
        pack_header1 header = {0};
        FILE* skill_pack = fopen(multiselectpath[n].c_str(), "rb");
        if (skill_pack == nullptr) {
            printf("Failed to open %s\n", multiselectpath[n].c_str());
            continue; // Skip to next mod file
        }
        fread(&header, sizeof(pack_header1), 1, skill_pack);

        skill_t* skills = new skill_t[header.skill_count];
        fread(skills, sizeof(skill_t), header.skill_count, skill_pack);
        for (int i = 0; i < header.skill_count; i++) {
            p.gstorage->skill_array[(skills[i].SkillID - 1)] = skills[i]; // Write skills from pack into gsdata
        }
        pack2_text* text_meta = (pack2_text*) malloc(sizeof(pack2_text) * header.skill_count);
        if (text_meta != nullptr) {
            fread(text_meta, sizeof(pack2_text), header.skill_count, skill_pack);

            // This is really hard to read. Sorry.
            if (header.format_version == 2) {
                for (uint16_t i = 0; i < header.skill_count; i++) {
                    skill_text original_text = load_skill_text(p, skills[i].SkillTextID + 1);

                    if (original_text.name.length() <= text_meta[i].name_length) {
                        int text_id = skills[i].SkillTextID + 1;
                        char* name = (char*)malloc(text_meta[i].name_length);
                        char* desc = (char*)malloc(text_meta[i].desc_length);
                        fread(name, text_meta[i].name_length, 1, skill_pack);
                        fread(desc, text_meta[i].desc_length, 1, skill_pack);
                        save_skill_text(p, { name, desc }, skills[i].SkillTextID + 1);
                        free(name);
                        free(desc);
                    }
                }
            }
        }
        // TODO: Test if freeing this breaks anything.
        // free(text_meta);
        delete[] skills;

        fclose(skill_pack);
    }
}

void toggle_game_pause(pd_meta p) {
    static bool GamePaused = false;
    if (GamePaused) {
        DebugActiveProcessStop(p.pid);
        GamePaused = true;
    } else {
        DebugActiveProcess(p.pid);
    }
    GamePaused = !GamePaused;
}