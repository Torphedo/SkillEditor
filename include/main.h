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

extern bool DebugMode;

// ===== UI Variables =====

extern short ErrorCode;
extern bool OptionsWindow;
extern bool RenderSkillPackWindow;
extern short AtkSkillState; // 0 = None, 1 = Opened, 2 = Saved
extern bool RenderAtkSkillWindow;

// ===== File Dialog Variables =====

extern string filepath;
extern string* multiselectpath;
extern int MultiSelectCount;

extern const COMDLG_FILTERSPEC skillfile[];
extern const COMDLG_FILTERSPEC skillpack[];

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
// void InstallSkillPack();
// int LoadGSDATA();
// int SaveGSDATA();

// ===== PDUWP Memory Editing =====

DWORD GetProcessIDByName(LPCTSTR ProcessName);
bool GetProcess();
char* GetAddressOfData(const uint8_t* data, size_t len);
int LoadGSDataFromRAM();
int SaveGSDataToRAM();
void InstallSkillPackToRAM();
void PauseGame();
void UnpauseGame();

// ===== Custom ImGui Functions / Wrappers =====

void Tooltip(const char* text);
void InputShort(const char* label, void* p_data);
void InputUInt8(const char* label, void* p_data);

// ===== Windows Explorer Dialogs =====

HRESULT MultiSelectWindow();
int WINAPI FileSelectDialog(const COMDLG_FILTERSPEC* fileTypes);
int WINAPI FileSaveDialog(const COMDLG_FILTERSPEC* fileTypes, LPCWSTR DefaultExtension);
