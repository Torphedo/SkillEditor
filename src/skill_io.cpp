#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdio.h>

#include "winAPI.h"
#include "memory-editing.h"

extern "C" {
#include <crc_32.h>
}


// Loads a file into the AtkSkill struct
AttackSkill load_attack_skill(char* filepath)
{
    AttackSkill skill_buffer = { 0 };
    FILE* skill_file = fopen(filepath, "rb");
    fread(&skill_buffer, sizeof(AttackSkill), 1, skill_file);
    fclose(skill_file);
    return skill_buffer;
}

// Writes the currently open file to disk.
void save_attack_skill(char* filepath)
{
    if (AtkSkill.SkillTextID != 0) // Check that we actually have data to write, this will always be > 0.
    {
        if (filepath != nullptr)
        {
            std::ofstream AtkSkillOut(filepath, std::ios::binary);

            AtkSkillOut.write((char*)&AtkSkill, 144);       // Overwrites the specified file with new data
            AtkSkillOut.close();
            std::cout << "Saved attack skill to " << filepath << "\n";
        }

        skillarray[(AtkSkill.SkillID - 1)] = AtkSkill; // Write skills from pack into gsdata (loaded in memory by LoadGSDATA())

        if (get_process())
        {
            // Only perform hash if the game is running
            gsdataheader.VersionNum = crc32buf((char*)&AtkSkill, 144);

            write_gsdata_to_memory();
            std::cout << "Wrote skill data to memory.\n";
            // To make it slightly harder for new users to figure out how the hashing works, it won't be printed out every time.
            // std::cout << "New version number: " << gsdataheader.VersionNum << "\n";
        }
    }
    else
    {
        std::cout << "Tried to save without opening a file.\n";
    }
}

void save_skill_pack(const char* packname)
{
    std::ofstream SkillPackOut(selected_filepath, std::ios::binary);
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
        if (std::filesystem::file_size(multiselectpath[i]) == 144) // Only allow skill data to be written if the skill file is the correct size
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
    std::cout << "Saved skill pack to " << selected_filepath << "\n";
}
