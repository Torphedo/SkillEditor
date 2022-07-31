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

void LoadAttackSkill(char* filename)
{
	AtkSkillFile.open(filename, ios::in | ios::binary);      // Open file
	AtkSkillFile.read((char*)&AtkSkill, (sizeof(AtkSkill))); // Read bytes into AttackSkill struct

	AtkSkillFile.close();
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
    return SelectedItemInt;
}