#include <filesystem>
#include <cstdio>

#include "winAPI.hxx"
#include "remote_pd.hxx"
#include "text.hxx"
#include "mods.hxx"

#include <common/crc32.h>

// Used to track the most recently saved attack skill filepath
char* most_recent_filename = nullptr;

bool skill_select() {
    most_recent_filename = file_save_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" }, L".skill");
    return most_recent_filename != nullptr;
}

unsigned int load_attack_skill(pd_meta p, unsigned int current_id) {
    most_recent_filename = file_select_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" });
    if (most_recent_filename != nullptr) {
        skill_t buffer = {0 };
        FILE* skill_file = fopen(most_recent_filename, "rb");
        fread(&buffer, sizeof(skill_t), 1, skill_file);

        p.gstorage->skill_array[buffer.SkillID - 1] = buffer;
        printf("Imported attack skill with ID %d from %s\n", buffer.SkillID, most_recent_filename);
        if (std::filesystem::file_size(most_recent_filename) > 144) {
            pack2_text text_meta = {0};
            fread(&text_meta, sizeof(pack2_text), 1, skill_file);

            skill_text original_text = load_skill_text(p, buffer.SkillTextID + 1);
            if (original_text.name.length() <= text_meta.name_length) {
                int text_id = buffer.SkillTextID + 1;
                char* name = (char*) malloc(text_meta.name_length);
                char* desc = (char*) malloc(text_meta.desc_length);
                fread(name, text_meta.name_length, 1, skill_file);
                fread(desc, text_meta.desc_length, 1, skill_file);
                save_skill_text(p, {name, desc}, buffer.SkillTextID + 1);
                free(name);
                free(desc);
            }
        }
        fclose(skill_file);
        return buffer.SkillID;
    }
    else {
        return current_id;
    }
}

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

            printf("Saved attack skill to %s\n", most_recent_filename);
        }
    }
    else {
        printf("Invalid skill ID.\n");
    }
}

void save_skill_pack(const char* packname) {
    // I believe we leak memory here, but it crashes if I free it for some reason...
    char* out_filepath = file_save_dialog(COMDLG_FILTERSPEC{ L"Skill Pack", L"*.bin;" }, L".bin");
    if (out_filepath != nullptr) {
        FILE* skill_pack_out = fopen(out_filepath, "wb");
        if (skill_pack_out != nullptr) {
            pack_header1 header = { 0 };
            bool has_text_data = false;
            header.skill_count = (short)MultiSelectCount;
            header.format_version = 2;
            memcpy(&header.name, packname, sizeof(header.name));

            skill_t* skills = (skill_t*) malloc(sizeof(skill_t) * header.skill_count);
            pack2_text* text = (pack2_text*) malloc(sizeof(pack2_text) * header.skill_count);
            char** text_data = (char**) calloc(header.skill_count * 2, sizeof(char*));
            for (int i = 0; i < MultiSelectCount; i++) {
                    FILE* skill_in = fopen(multiselectpath[i].c_str(), "rb");
                    if (skill_in != nullptr) {
                        fread(&skills[i], sizeof(skill_t), 1, skill_in);
                        if (std::filesystem::file_size(multiselectpath[i]) > 144) {
                            has_text_data = true;
                            fread(&text[i], sizeof(pack2_text), 1, skill_in);
                            text_data[(i * 2)] = (char*) malloc(text[i].name_length);
                            text_data[(i * 2) + 1] = (char*) malloc(text[i].desc_length);
                            fread(text_data[(i * 2)], text[i].name_length, 1, skill_in);
                            fread(text_data[(i * 2) + 1], text[i].desc_length, 1, skill_in);
                        }
                        fclose(skill_in);
                    }

                    printf("Writing from %s...\n", multiselectpath[i].c_str());
            }

            if (!has_text_data) { header.format_version = 1; }
            fwrite(&header, sizeof(pack_header1), 1, skill_pack_out);
            fwrite(skills, sizeof(skill_t), header.skill_count, skill_pack_out);

            if (has_text_data) {
                fwrite(text, sizeof(pack2_text), header.skill_count, skill_pack_out);
                for (int i = 0; i < header.skill_count; i++) {
                    if (text_data[i * 2] != nullptr) {
                        fwrite(text_data[i * 2], text[i * 2].name_length, 1, skill_pack_out);
                        free(text_data[i * 2]);
                    }
                    if (text_data[(i * 2) + 1] != nullptr) {
                        fwrite(text_data[(i * 2) + 1], text[i * 2].desc_length, 1, skill_pack_out);
                        free(text_data[(i * 2) + 1]);
                    }
                }
            }
            free(text);
            free(skills);

            fclose(skill_pack_out);
        }
        printf("Saved skill pack to %s\n", out_filepath);
    }
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