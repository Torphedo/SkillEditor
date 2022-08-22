#include <algorithm>
#include <vector>

#include <main.h>
#include <UI.h>
#include "winAPI.h"
#include "memory-editing.h"

extern "C" {
#include <crc32/crc_32.h>
}

using std::ios, std::cout;

bool DebugMode = false;

// ===== User Input Variables =====

char packname[32];
char PhantomDustDir[275];


int main()
{
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    CreateUI(); // Main UI loop, see UI.h.
    CoUninitialize();
    return 0;
}

// ===== File I/O =====

// Loads the current file into the AtkSkill struct
void LoadAttackSkill()
{
	AtkSkillFile.open(filepath, ios::in | ios::binary);   // Open file
	AtkSkillFile.read((char*)&AtkSkill, (sizeof(AtkSkill))); // Read bytes into AttackSkill struct

	AtkSkillFile.close();
	return;
}

// Writes the currently open file to disk.
void SaveAtkSkill()
{
    if (OpenedAttackSkill)
    {
        std::ofstream AtkSkillOut(filepath, ios::binary); // Creates a new ofstream variable, using
                                                             // the name of the file that was opened.

        AtkSkillOut.write((char*)&AtkSkill, 144);       // Overwrites the file that was opened with
                                                        // the new data.
        AtkSkillOut.close();
        cout << "Saved attack skill to " << filepath << "\n";

        skillarray[(AtkSkill.SkillID - 1)] = AtkSkill; // Write skills from pack into gsdata (loaded in memory by LoadGSDATA())

        if (GetProcess())
        {
            // Only perform hash if the game is running
            gsdataheader.VersionNum = crc32buf((char*)&AtkSkill, 144);

            SaveGSDataToRAM();
            cout << "Wrote skill data to memory.\n";
            // To make it slightly harder for new users to figure out how the hashing works, it won't be printed out every time.
            // cout << "New version number: " << gsdataheader.VersionNum << "\n";
        }
    }
    else
    {
        cout << "Tried to save without opening a file, aborting...\n";
    }
}

void SaveSkillPack()
{
    std::ofstream SkillPackOut(filepath, ios::binary);
    std::fstream SkillStream; // fstream for the skill files selected by the user
    char skilldata[144]; // 144 byte buffer to read / write skill data from
    short FormatVersion = 1;
    short SkillCount = (short) MultiSelectCount; // Skill count == # of files selected
    int pad[3] = { 0,0,0 };

    SkillPackOut.write((char*)&packname, sizeof(packname)); // 32 characters for the name
    SkillPackOut.write((char*)&FormatVersion, sizeof(FormatVersion)); // This version is v1
    SkillPackOut.write((char*)&SkillCount, sizeof(SkillCount)); // So we know when to stop
    SkillPackOut.write((char*)&pad, 12); // Better alignment makes the file easier to read

    for (int i = 0; i < MultiSelectCount; i++)
    {
        if (std::filesystem::file_size(multiselectpath[i]) == 144) // Only allow skill data to be written if the skill file is the correct size
        {
            SkillStream.open(multiselectpath[i], ios::in | ios::binary);
            SkillStream.read((char*)&skilldata, 144); // Read skill data from the file to our skilldata buffer
            SkillStream.seekg(0);
            cout << "Writing from " << multiselectpath[i] << "...\n";
            SkillPackOut.write((char*)&skilldata, 144); // Writing to skill pack
        }
        else
        {
            cout << "Invalid skill filesize. Write skipped.\n";
        }
    }
    SkillStream.close();

    SkillPackOut.close();
    cout << "Saved skill pack to " << filepath << "\n";
}

// void InstallSkillPack()
// {
//     if (LoadGSDATA() == 0)
//     {
//         int* Filesize;
//         Filesize = new int[MultiSelectCount];
//         fstream SkillPackBlob; // Separate stream that will only have skill pack data, so that we can just pass it as a buffer to be hashed.
//                                // This is way more efficient than writing them all to a single file on disk, hashing that, then deleting it.
//         int BlobSize = 0;
//         for (unsigned int n = 0; n < MultiSelectCount; n++) // Loop through every selected skill pack file
//         {
//             fstream SkillPackIn;
//             SkillPackHeaderV1 header;
//             SkillPackIn.open(multiselectpath[n], ios::in | ios::binary);
//             SkillPackIn.read((char*)&header, sizeof(header));
//             Filesize[n] = (int) std::filesystem::file_size(multiselectpath[n]);
// 
//             for (int i = 0; i < header.SkillCount; i++)
//             {
//                 atkskill skill; // Instance of struct. ID will be in the same posiiton every time, so it's fine to use the attack template.
//                 SkillPackIn.read((char*)&skill, sizeof(skill)); // Read a skill into struct
//                 skillarray[(skill.SkillID - 1)] = skill; // Write skills from pack into gsdata (loaded in memory by LoadGSDATA())
//             }
//         }
//         for (unsigned int n = 0; n < MultiSelectCount; n++)
//         {
//             BlobSize += Filesize[n];
//         }
//         SkillPackBlobData = new char[BlobSize];
//         for (unsigned int n = 0; n < MultiSelectCount; n++)
//         {
//             SkillPackBlob.open(multiselectpath[n], ios::in | ios::binary);
//             SkillPackBlob.read(SkillPackBlobData, BlobSize);
//         }
//         SkillPackBlob.close();
// 
//         uint32_t hash = crc32buf(SkillPackBlobData, BlobSize);
//         gsdataheader.VersionNum = (int) hash;
//         cout << hash << "\n";
// 
//         SaveGSDATA();
//     }
// }
// 
// int LoadGSDATA()
// {
//     filesystem::path gsdatapath{ (PhantomDustDir + (string)"\\Assets\\Data\\gstorage\\gsdata_en.dat") };
//     if (!filesystem::exists(gsdatapath))
//     {
//         cout << "Invalid Phantom Dust folder.\n";
//         return 1; // Failure
//     }
// 
//     GSDataStream.open(gsdatapath, ios::in | ios::binary); // Open file
//     GSDataStream.read((char*)&gsdataheader, (sizeof(gsdataheader))); // Read bytes into gsdataheader struct
// 
//     GSDataStream.read((char*)skillarray, (sizeof(skillarray)));
// 
//     int datasize = (sizeof(gsdataheader) + sizeof(skillarray)) / 4;
// 
//     gsdatamain = new int[((gsdataheader.Filesize - datasize))];
//     GSDataStream.read((char*)gsdatamain, (datasize * 4)); // Multiplied by 4 because each int is 4 bytes
// 
//     GSDataStream.close();
//     return 0; // Success
// }
// 
// int SaveGSDATA()
// {
//     filesystem::path gsdatapath{ (PhantomDustDir + (string)"\\Assets\\Data\\gstorage\\gsdata_en.dat") };
//     if (!filesystem::exists(gsdatapath))
//     {
//         cout << "Invalid Phantom Dust folder.\n";
//         return 1; // Failure
//     }
//     ofstream GSDataOut(gsdatapath, ios::binary); // Creates a new ofstream variable, using
//                                                  // the name of the file that was opened.
// 
//     GSDataOut.write((char*)&gsdataheader, 160);  // Overwrites the file that was opened with
// 
//     GSDataOut.write((char*)&skillarray, sizeof(skillarray));  // Overwrites the file that was opened with
// 
//     GSDataOut.write((char*)gsdatamain, (gsdataheader.Filesize - 108304));
//     GSDataOut.close();
//     return 0; // Success
// }

// ===== Custom ImGui Functions / Wrappers =====

void Tooltip(const char* text)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", text);
    }
    ImGui::TableNextColumn();
}

// ImGui didn't have pre-made functions for short
// or uint8 input boxes, so I made my own.

const short s8_one = 1;
const short s16_one = 1;
void InputShort(const char* label, void* p_data)
{
    ImGui::SetNextItemWidth(200);
    ImGui::InputScalar(label, ImGuiDataType_S16, p_data, true ? &s16_one : NULL, NULL, "%d");
}

void InputUInt8(const char* label, void* p_data)
{
    ImGui::SetNextItemWidth(200);
    ImGui::InputScalar(label, ImGuiDataType_S8, p_data, true ? &s8_one : NULL, NULL, "%d");
}
