#include <Windows.h>
#include <shobjidl.h> 
#include <string>
#include <iostream>
#include <fstream>
#include <thread>

#include <main.h>
#include <setupUI.h>
using namespace std;

int main()
{
    std::thread CoInitThread(COM_Init); // Multithreaded CoInitialize so that it doesn't slow down the UI init.
    CoInitThread.join();

    CreateUI(); // Main UI loop, see setupUI.h.
    CoUninitialize();
}


// ===== File I/O =====

// Loads the current file into the AtkSkill struct
void LoadAttackSkill()
{
	AtkSkillFile.open(filepathptr, ios::in | ios::binary);   // Open file
	AtkSkillFile.read((char*)&AtkSkill, (sizeof(AtkSkill))); // Read bytes into AttackSkill struct

	AtkSkillFile.close();
	return;
}

// Writes the currently open file to disk.
void SaveAtkSkill()
{
    ofstream AtkSkillOut(filepathptr, ios::binary); // Creates a new ofstream variable, using
                                                    // the name of the file that was opened.

    AtkSkillOut.write((char*)&AtkSkill, 144);       // Overwrites the file that was opened with
                                                    // the new data.
    AtkSkillOut.close();
}

void SaveSkillPack()
{
    ofstream SkillPackOut(filepathptr, ios::binary);
    short FormatVersion = 1;
    short SkillCount = (short) MultiSelectCount; // Skill count == # of files selected
    int pad[3] = { 0,0,0 };

    SkillPackOut.write((char*)&packname, sizeof(packname)); // 32 characters for the name
    SkillPackOut.write((char*)&FormatVersion, sizeof(FormatVersion)); // This version is v1
    SkillPackOut.write((char*)&SkillCount, sizeof(SkillCount)); // So we know when to stop
    SkillPackOut.write((char*)&pad, 12); // Better alignment makes the file easier to read

    for (DWORD i = 0; i < MultiSelectCount; i++)
    {
        int size = (int) std::filesystem::file_size(multiselectpath[i]);
        if (size == 144) // Only allow skill data to be written if the skill file is the correct size
        {
            fstream SkillStream; // fstream for the skill files selected by the user
            char skilldata[144]; // 144 byte buffer to read / write skill data from
            SkillStream.open(multiselectpath[i], ios::in | ios::binary);
            SkillStream.read((char*)&skilldata, 144); // Read skill data from the file to our skilldata buffer
            cout << "Writing from " << multiselectpath[i] << "...\n";
            SkillPackOut.write((char*)&skilldata, 144); // Writing to skill pack
            SkillStream.close();
        }
        else
        {
            cout << "Invalid skill filesize. Write skipped.";
        }
    }

    SkillPackOut.close();
    cout << "Saved skill pack to " << filepath << "\n";
    SkillPackWindow = false;
}

void InstallSkillPack()
{
    if (LoadGSDATA() == 0)
    {
        int* Filesize;
        Filesize = new int[MultiSelectCount];
        fstream SkillPackBlob; // Separate stream that will only have skill pack data, so that we can just pass it as a buffer to be hashed.
                               // This is way more efficient than writing them all to a single file on disk, hashing that, then deleting it.
        int BlobSize = 0;
        for (unsigned int n = 0; n < MultiSelectCount; n++) // Loop through every selected skill pack file
        {
            fstream SkillPackIn;
            SkillPackHeaderV1 header;
            SkillPackIn.open(multiselectpath[n], ios::in | ios::binary);
            SkillPackIn.read((char*)&header, sizeof(header));
            Filesize[n] = (int) std::filesystem::file_size(multiselectpath[n]);

            for (int i = 0; i < header.SkillCount; i++)
            {
                atkskill skill; // Instance of struct. ID will be in the same posiiton every time, so it's fine to use the attack template.
                SkillPackIn.read((char*)&skill, sizeof(skill)); // Read a skill into struct
                skillarray[(skill.SkillID - 1)] = skill; // Write skills from pack into gsdata (loaded in memory by LoadGSDATA())
            }
        }
        for (unsigned int n = 0; n < MultiSelectCount; n++)
        {
            BlobSize += Filesize[n];
        }
        SkillPackBlobData = new char[BlobSize];
        for (unsigned int n = 0; n < MultiSelectCount; n++)
        {
            SkillPackBlob.open(multiselectpath[n], ios::in | ios::binary);
            SkillPackBlob.read(SkillPackBlobData, BlobSize);
        }
        SkillPackBlob.close();

        uint32_t hash = crc32buf(SkillPackBlobData, BlobSize);
        gsdataheader.VersionNum = (int) hash;
        cout << hash << "\n";

        SaveGSDATA();
    }
}

int LoadGSDATA()
{
    filesystem::path gsdatapath{ (PhantomDustDir + (string)"\\Assets\\Data\\gstorage\\gsdata_en.dat") };
    if (!filesystem::exists(gsdatapath))
    {
        cout << "Invalid Phantom Dust folder.\n";
        return 1; // Failure
    }

    GSDataStream.open(gsdatapath, ios::in | ios::binary); // Open file
    GSDataStream.read((char*)&gsdataheader, (sizeof(gsdataheader))); // Read bytes into gsdataheader struct

    GSDataStream.read((char*)skillarray, (sizeof(skillarray)));

    int datasize = (sizeof(gsdataheader) + sizeof(skillarray)) / 4;

    gsdatamain = new int[((gsdataheader.Filesize - datasize))];
    GSDataStream.read((char*)gsdatamain, (datasize * 4)); // Multiplied by 4 because each int is 4 bytes

    GSDataStream.close();
    return 0; // Success
}

int SaveGSDATA()
{
    filesystem::path gsdatapath{ (PhantomDustDir + (string)"\\Assets\\Data\\gstorage\\gsdata_en.dat") };
    if (!filesystem::exists(gsdatapath))
    {
        cout << "Invalid Phantom Dust folder.\n";
        return 1; // Failure
    }
    ofstream GSDataOut(gsdatapath, ios::binary); // Creates a new ofstream variable, using
                                                 // the name of the file that was opened.

    GSDataOut.write((char*)&gsdataheader, 160);  // Overwrites the file that was opened with

    GSDataOut.write((char*)&skillarray, sizeof(skillarray));  // Overwrites the file that was opened with

    GSDataOut.write((char*)gsdatamain, (gsdataheader.Filesize - 108304));
    GSDataOut.close();
    return 0; // Success
}

// ===== Custom ImGui Functions / Wrappers =====

void Tooltip(const char* text) {
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("%s", text);
}

// ImGui didn't have pre-made functions for short
// or uint8 input / combo boxes, so I made my own.

void InputShort(const char* label, void* p_data)
{
    const short s16_one = 1;
    ImGui::InputScalar(label, ImGuiDataType_S16, p_data, true ? &s16_one : NULL, NULL, "%d");
}

void InputUInt8(const char* label, void* p_data) {
    const short s8_one = 1;
    ImGui::InputScalar(label, ImGuiDataType_S8, p_data, true ? &s8_one : NULL, NULL, "%d");
}

short ComboShort(const char* label, const char* const* items, int item_count)
{
    static int SelectedItemInt;
    ImGui::Combo(label, &SelectedItemInt, items, item_count);
    return (short)SelectedItemInt; // Explicit conversion to avoid compiler warning
}
