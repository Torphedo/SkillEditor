
#include <windows.h>

extern "C" {
#include "crc_32.h"
}
#include "memory-editing.h"

#include "types.h"
#include "structs.h"
#include "text.h"
#include "winAPI.h"

// TODO: This is some janky bullshit.
skill_text load_skill_text(pd_meta p, unsigned int id) {
    if (p.h == INVALID_HANDLE_VALUE || p.h == NULL) {
        return {"", ""};
    }

    // Memory address of the skill text table & strings
    // static uintptr_t text_section = gstorage_address + (gstorage.unk0 & 0xFFFFF000) + 0x1A000;
    static uintptr_t text_section = p.gstorage_addr + 0x34000;

    // Read section header to check the number of strings
    text_header header = { 0 };
    ReadProcessMemory(p.h, (LPVOID)text_section, &header, sizeof(text_header), NULL);
    if (GetLastError() != 0) {
        printf("Failed to read process memory (code %li)\n", GetLastError());
        return { "", ""};
    }

    if (id > header.array_size - 1) {
        return { "No text found.", "No text found." };
    }

    uintptr_t text_ptr_location = text_section + sizeof(text_header) + ((id - 1) * 0xC);

    text_ptrs string_offset[2] = {0};
    ReadProcessMemory(p.h, (LPVOID)text_ptr_location, &string_offset, sizeof(text_ptrs) * 2, NULL);
    if (GetLastError() != 0) {
        printf("Failed to read process memory (code %li)\n", GetLastError());
        return { "", "" };
    }

    uint32_t name_size = string_offset[0].desc - string_offset[0].name;
    uint32_t desc_size = string_offset[1].name - string_offset[0].desc + sizeof(text_ptrs);
    char* name = (char*)calloc(1, name_size);
    char* desc = (char*)calloc(1, desc_size);

    ReadProcessMemory(p.h, (LPVOID)(text_ptr_location + string_offset[0].name), name, name_size, NULL);
    if (GetLastError() != 0) {
        printf("Failed to read process memory (code %li)\n", GetLastError());
        return { "", "" };
    }
    ReadProcessMemory(p.h, (LPVOID)(text_ptr_location + string_offset[0].desc), desc, desc_size, NULL);
    if (GetLastError() != 0) {
        printf("Failed to read process memory (code %li)\n", GetLastError());
        return { "", "" };
    }

    skill_text output = { name, desc };
    free(name);
    free(desc);

    return output;
}

// TODO: This is also some janky bullshit.
bool save_skill_text(pd_meta p, skill_text text, unsigned int id) {
    if (p.h == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Memory address of the skill text table & strings
    // static uintptr_t text_section = gstorage_address + (gstorage.unk0 & 0xFFFFF000) + 0x1A000;
    static uintptr_t text_section = p.gstorage_addr + 0x34000;

    // Read section header to check the number of strings
    text_header header = { 0 };
    ReadProcessMemory(p.h, (LPVOID)text_section, &header, sizeof(text_header), NULL);
    if (GetLastError() != 0) {
        printf("Failed to read process memory (code %li)\n", GetLastError());
        return false;
    }

    if (id > header.array_size - 1) {
        return false;
    }

    uintptr_t text_ptr_location = text_section + sizeof(text_header) + ((id - 1) * 0xC);

    text_ptrs string_offset[2] = {0};
    ReadProcessMemory(p.h, (LPVOID)text_ptr_location, &string_offset, sizeof(text_ptrs) * 2, NULL);
    if (GetLastError() != 0) {
        printf("Failed to read process memory (code %li)\n", GetLastError());
        return false;
    }

    uint32_t name_size = text.name.length() + 1;
    uint32_t desc_size = text.desc.length() + 1;
    char* name = text.name.data();
    char* desc = text.desc.data();

    WriteProcessMemory(p.h, (LPVOID)(text_ptr_location + string_offset[0].name), name, name_size, NULL);
    if (GetLastError() != 0) {
        printf("Failed to write process memory (code %li)\n", GetLastError());
        return false;
    }
    WriteProcessMemory(p.h, (LPVOID)(text_ptr_location + string_offset[0].desc), desc, desc_size, NULL);
    if (GetLastError() != 0) {
        printf("Failed to write process memory (code %li)\n", GetLastError());
    }

    return true;
}

bool write_gsdata_to_memory(pd_meta p) {
    if (can_read_memory(p)) {
        // Update version number
        p.gstorage->VersionNum = crc32buf((char*)p.gstorage, sizeof(*p.gstorage));

        WriteProcessMemory(p.h, (LPVOID)p.gstorage_addr, p.gstorage, sizeof(*p.gstorage), NULL);
        DWORD error = GetLastError();
        if (error != 1400 && error != 183 && error != 0) {
            printf("Process Write Error Code: %ld\n", error);
            return false;
        }
        else {
            printf("Wrote skill data to memory.\n");
            return true;
        }
    }
    else { return false; }
}

void install_mod(pd_meta p) {
    if (can_read_memory(p)) {
        for (int n = 0; n < MultiSelectCount; n++) { // Loop through every selected skill pack file
            pack_header1 header = {0};
            FILE* skill_pack = fopen(multiselectpath[n].c_str(), "rb");
            if (skill_pack == nullptr) {
                printf("Failed to open %s\n", multiselectpath[n].c_str());
                continue; // Skip to next mod file
            }
            fread(&header, sizeof(pack_header1), 1, skill_pack);

            atkskill* skills = new atkskill[header.skill_count];
            fread(skills, sizeof(atkskill), header.skill_count, skill_pack);
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

        write_gsdata_to_memory(p); // Automatically updates version number
    }
}

void toggle_game_pause(pd_meta p) {
    static bool GamePaused = false;
    if (GamePaused) {
        DebugActiveProcessStop(p.pid);
        GamePaused = true;
    }
    else {
        DebugActiveProcess(p.pid);
    }
    GamePaused = !GamePaused;
}
