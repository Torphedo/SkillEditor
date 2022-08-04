#pragma once

#include <Windows.h>
#include <shobjidl.h> 
#include <stdint.h>
#include <string>
#include <fstream>
#include <string>
#include <iostream>
#include <thread>
#include <tchar.h>
#include <filesystem>

#include <imgui_internal.h>
#include <structs.h>


extern "C" {
#include <crc32/crc_32.c>
}

using namespace std;

// ===== File Dialog Variables =====

const COMDLG_FILTERSPEC fileTypes[] = { L"Skill File", L"*.skill;" };
HRESULT hr;
string filepath;
char* filepathptr;
string* multiselectpath;
DWORD MultiSelectCount;

// ===== File I/O Variables & Shared Data =====

fstream AtkSkillFile; // fstream for Attack Skill files
AttackSkill AtkSkill; // Attack skill struct
fstream GSDataStream; // fstream for gsdata
GSDataHeader gsdataheader; // First 160 bytes of gsdata
atkskill skillarray[751];  // Array of 751 skill data blocks
int* gsdatamain; // Text, whitespace, other data shared among gsdata save/load functions
char* SkillPackBlobData;

// ===== User Input Variables =====

char packname[32];
char PhantomDustDir[275];

// ===== UI Variables =====
bool OptionsWindow = false;
bool SkillPackWindow = false;
short AtkSkillState = 0; // 0 = None, 1 = Opened, 2 = Saved
bool AtkSkillWindow = false;



// ========== Custom Functions ==========


string PWSTR_to_string(PWSTR ws) {
    string result;
    result.reserve(wcslen(ws));
    for (; *ws; ws++)
        result += (char)*ws;
    return result;
}

// ===== File I/O =====

void LoadAttackSkill();
void SaveAtkSkill();
void SaveSkillPack();
void InstallSkillPack();
int LoadGSDATA();
int SaveGSDATA();

// ===== Custom ImGui Functions / Wrappers =====

void Tooltip(const char* text);
void InputShort(const char* label, void* p_data);
void InputUInt8(const char* label, void* p_data);
short ComboShort(const char* label, const char* const* items, int item_count);

// ===== Windows Explorer Dialogs =====

HRESULT MultiSelectWindow();
int WINAPI FileSelectDialog();
int WINAPI FileSaveDialog();
