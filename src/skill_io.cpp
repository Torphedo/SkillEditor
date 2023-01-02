#include <filesystem>
#include <stdio.h>

#include "winAPI.h"
#include "memory-editing.h"

extern "C" {
#include <crc_32.h>
}

// Used to track the most recently saved attack skill filepath
char* most_recent_filename = nullptr;

unsigned int load_attack_skill(unsigned int current_id)
{
    most_recent_filename = file_select_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" });
    if (most_recent_filename != nullptr)
    {
        if (std::filesystem::file_size(most_recent_filename) == 144)
        {
            atkskill buffer = { 0 };
            FILE* skill_file = fopen(most_recent_filename, "rb");
            fread(&buffer, sizeof(atkskill), 1, skill_file);
            fclose(skill_file);

            gstorage.skill_array[buffer.SkillID - 1] = buffer;
            printf("Imported attack skill with ID %d from %s\n", buffer.SkillID, most_recent_filename);
            return buffer.SkillID - 1;
        }
        else { printf("Selected file was not a valid skill.\n"); return current_id; }
    }
    else { return current_id; }
}

// Writes the currently open file to disk.
void save_skill_to_file(unsigned int id)
{
    // Check that we actually have data to write and a place to write it to
    if (gstorage.skill_array[id - 1].SkillID != 0)
    {
        if (most_recent_filename == nullptr)
        {
            most_recent_filename = file_select_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" });
        }
        if (most_recent_filename != nullptr) // Must be checked again in case user cancels selection
        {
            FILE* skill_out = fopen(most_recent_filename, "wb");
            fwrite(&gstorage.skill_array[id - 1], sizeof(atkskill), 1, skill_out);
            fclose(skill_out);

            printf("Saved attack skill to %s\n", most_recent_filename);
        }
        write_gsdata_to_memory();
    }
    else
    {
        printf("Invalid skill ID.\n");
    }
}

void save_skill_with_dialog(unsigned int id)
{
    most_recent_filename = file_save_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" }, L".skill");
    save_skill_to_file(id);
}

void save_skill_pack(const char* packname)
{
    // I believe we leak memory here, but it crashes if I free it for some reason...
    char* out_filepath = file_save_dialog(COMDLG_FILTERSPEC{ L"Skill Pack", L"*.bin;" }, L".bin");
    if (out_filepath != nullptr)
    {
        FILE* skill_pack_out = fopen(out_filepath, "wb");
        if (skill_pack_out != nullptr)
        {
            packheader1 header = { 0 };
            header.SkillCount = (short)MultiSelectCount;
            header.FormatVersion = 1;
            strcpy(header.Name, packname);
            atkskill* skills = new atkskill[header.SkillCount];
            fwrite(&header, sizeof(packheader1), 1, skill_pack_out);

            for (int i = 0; i < MultiSelectCount; i++)
            {
                // Only allow skill data to be written if the skill file is the correct size
                if (std::filesystem::file_size(multiselectpath[i]) == 144)
                {
                    FILE* skill_in = fopen(multiselectpath[i].c_str(), "rb");
                    if (skill_in != nullptr) {
                        fread(&skills[i], sizeof(atkskill), 1, skill_in);
                        fclose(skill_in);
                    }

                    printf("Writing from %s...\n", multiselectpath[i].c_str());
                }
                else
                {
                    printf("Invalid skill size, file skipped.\n");
                }
            }
            fwrite(skills, sizeof(atkskill), header.SkillCount, skill_pack_out);
            delete[] skills;

            fclose(skill_pack_out);
        }
        printf("Saved skill pack to %s\n", out_filepath);
    }
}
