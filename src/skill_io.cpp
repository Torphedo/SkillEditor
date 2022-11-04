#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdio.h>

#include "winAPI.h"
#include "memory-editing.h"

extern "C" {
#include <crc_32.h>
}

// Used to track the most recently saved attack skill filepath
char* most_recent_filename;

// Loads a file into the AtkSkill struct
AttackSkill load_attack_skill()
{
    AttackSkill skill_buffer = { 0 };
    char* filepath_in = file_select_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" });
    if (filepath_in != nullptr)
    {
        FILE* skill_file = fopen(filepath_in, "rb");
        fread(&skill_buffer, sizeof(AttackSkill), 1, skill_file);
        fclose(skill_file);
        std::cout << "Imported attack skill " << filepath_in << "\n";
        strcpy(most_recent_filename, filepath_in);
        free(filepath_in);
    }
    return skill_buffer;
}

// Writes the currently open file to disk.
void save_attack_skill()
{
    if (AtkSkill.SkillTextID != 0) // Check that we actually have data to write, this will always be > 0.
    {
        std::ofstream AtkSkillOut(most_recent_filename, std::ios::binary);
        AtkSkillOut.write((char*)&AtkSkill, 144);       // Overwrites the specified file with new data
        AtkSkillOut.close();
        std::cout << "Saved attack skill to " << most_recent_filename << "\n";

        gstorage.skill_array[(AtkSkill.SkillID - 1)] = AtkSkill; // Write skills from pack into gsdata (loaded in memory by LoadGSDATA())

        // Only perform hash if the game is running
        if (get_process())
        {
            // Update version number
            gstorage.VersionNum = crc32buf((char*)&AtkSkill, 144);

            write_gsdata_to_memory();
            std::cout << "Wrote skill data to memory.\n";
        }
    }
    else
    {
        std::cout << "Tried to save without opening a file.\n";
    }
}

void save_attack_skill_with_file_select()
{
    char* out_filepath = file_save_dialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" }, L".skill");
    if (out_filepath != nullptr)
    {
        most_recent_filename = out_filepath;
        free(out_filepath);
        save_attack_skill();
    }
}

void save_skill_pack(const char* packname)
{
    char* out_filepath = file_save_dialog(COMDLG_FILTERSPEC{ L"Skill Pack", L"*.bin;" }, L".bin");
    if (out_filepath != nullptr)
    {
        std::ofstream SkillPackOut(out_filepath, std::ios::binary);
        std::fstream SkillStream; // fstream for the skill files selected by the user
        char skilldata[144]; // 144 byte buffer to read / write skill data from
        short FormatVersion = 1;
        short SkillCount = (short)MultiSelectCount; // Skill count == # of files selected
        int pad[3] = { 0,0,0 };

        SkillPackOut.write(packname, 32); // 32 characters for the name
        SkillPackOut.write((char*)&FormatVersion, sizeof(FormatVersion)); // This version is v1
        SkillPackOut.write((char*)&SkillCount, sizeof(SkillCount)); // So we know when to stop
        SkillPackOut.write((char*)&pad, 12); // Better alignment makes the file easier to read

        for (int i = 0; i < MultiSelectCount; i++)
        {
            // Only allow skill data to be written if the skill file is the correct size
            if (std::filesystem::file_size(multiselectpath[i]) == 144)
            {
                SkillStream.open(multiselectpath[i], std::ios::in | std::ios::binary);
                SkillStream.read((char*)&skilldata, 144); // Read skill data from the file to our skilldata buffer
                SkillStream.seekg(0);
                std::cout << "Writing from " << multiselectpath[i] << "...\n";
                SkillPackOut.write((char*)&skilldata, 144); // Writing to skill pack
            }
            else
            {
                std::cout << "Invalid skill filesize. Write skipped.\n";
            }
        }
        SkillStream.close();

        SkillPackOut.close();
        std::cout << "Saved skill pack to " << out_filepath << "\n";
        free(out_filepath);
    }
}
