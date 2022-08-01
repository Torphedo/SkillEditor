#include <Windows.h>
#include <shobjidl.h> 
#include <string>
#include <iostream>
#include <fstream>

#include <main.h>
#include <setupUI.h>
using namespace std;

int main()
{
    CreateUI(); // Main UI loop, see setupUI.h.
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

void LoadGSDATA()
{
    char gsdatapath[275];
    strcpy_s(gsdatapath, (PhantomDustDir + "\\Assets\\Data\\gstorage\\gsdata_en.bin").c_str());

    GSDataStream.open(gsdatapath, ios::in | ios::binary); // Open file
    GSDataStream.read((char*)&gsdataheader, (sizeof(gsdataheader))); // Read bytes into gsdataheader struct

    char test[] = "test";
    uint32_t hash = crc32buf(test, (IM_ARRAYSIZE(test) - 1));
    cout << hash << "\n";

    GSDataStream.read((char*)skillarray, (sizeof(skillarray)));

    gsdatamain = new int[((gsdataheader.Filesize - 108304)/4)];
    GSDataStream.read((char*)gsdatamain, (sizeof(int) * (gsdataheader.Filesize - 108304) / 4));

    GSDataStream.close();
}

void SaveGSDATA()
{
    char gsdatapath[275];
    strcpy_s(gsdatapath, (PhantomDustDir + "\\Assets\\Data\\gstorage\\data.bin").c_str());
    ofstream GSDataOut(gsdatapath, ios::binary); // Creates a new ofstream variable, using
                                                 // the name of the file that was opened.

    GSDataOut.write((char*)&gsdataheader, 160);  // Overwrites the file that was opened with

    GSDataOut.write((char*)&skillarray, sizeof(skillarray));  // Overwrites the file that was opened with

    GSDataOut.write((char*)gsdatamain, (gsdataheader.Filesize - 108304));
    GSDataOut.close();
    return;
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