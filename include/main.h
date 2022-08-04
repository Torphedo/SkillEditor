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

#include <tlhelp32.h>

#include <imgui_internal.h>
#include <structs.h>

using namespace std;

// ===== File Dialog Variables =====

extern string filepath;
extern string* multiselectpath;
extern DWORD MultiSelectCount;

// ===== File I/O Variables & Shared Data =====

extern AttackSkill AtkSkill; // Attack skill struct

// ===== User Input Variables =====

extern char packname[32];
extern char PhantomDustDir[275];

// ========== Custom Functions ==========


string PWSTR_to_string(PWSTR ws);

// ===== File I/O =====

void LoadAttackSkill();
void SaveAtkSkill();
void SaveSkillPack();
void InstallSkillPack();
int LoadGSDATA();
int SaveGSDATA();

// ===== Debugging the PDUWP Process =====

DWORD GetProcessIDByName(LPCTSTR ProcessName);
void AttachToProcess();
int LoadGSDataFromRAM();
int SaveGSDataToRAM();
void InstallSkillPackToRAM();
void PauseGame();
void UnpauseGame();

// ===== Custom ImGui Functions / Wrappers =====

void Tooltip(const char* text);
void InputShort(const char* label, void* p_data);
void InputUInt8(const char* label, void* p_data);
short ComboShort(const char* label, const char* const* items, int item_count);

// ===== Windows Explorer Dialogs =====

HRESULT MultiSelectWindow();
int WINAPI FileSelectDialog();
int WINAPI FileSaveDialog();
