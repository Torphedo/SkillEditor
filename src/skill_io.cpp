#include <filesystem>
#include <stdio.h>

#include "winAPI.h"
#include "memory-editing.h"

extern "C" {
#include <crc_32.h>
}

// Used to track the most recently saved attack skill filepath
char* most_recent_filename;

// Loads a file into the AtkSkill struct
atkskill load_attack_skill()
{
    atkskill skill_buffer = { 0 };
    most_recent_filename = file_select_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" });
    if (most_recent_filename != nullptr)
    {
        FILE* skill_file = fopen(most_recent_filename, "rb");
        fread(&skill_buffer, sizeof(atkskill), 1, skill_file);
        fclose(skill_file);
        printf("Imported attack skill %s\n", most_recent_filename);
    }
    return skill_buffer;
}

// Writes the currently open file to disk.
void save_attack_skill(atkskill skill)
{
    if (skill.SkillTextID != 0) // Check that we actually have data to write, this should always be > 0.
    {
        FILE* skill_out = fopen(most_recent_filename, "wb");
        fwrite(&skill, sizeof(atkskill), 1, skill_out);
        fclose(skill_out);

        printf("Saved attack skill to %s\n", most_recent_filename);

        // Only perform hash and update gstorage if the game is running
        if (get_process() && load_skill_data())
        {
            // Write skill into gsdata
            gstorage.skill_array[(skill.SkillID - 1)] = skill;

            // Update version number
            gstorage.VersionNum = crc32buf((char*)&skill, 144);

            write_gsdata_to_memory();
            printf("Wrote skill data to memory.\n");
        }
    }
    else
    {
        printf("Tried to save without opening a file.\n");
    }
}

void save_attack_skill_with_file_select(atkskill skill)
{
    most_recent_filename = file_save_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" }, L".skill");
    save_attack_skill(skill);
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
