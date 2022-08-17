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

extern bool DebugMode;

// ===== UI Variables =====

extern short ErrorCode;
extern bool OptionsWindow;
extern bool RenderSkillPackWindow;
extern short AtkSkillState; // 0 = None, 1 = Opened, 2 = Saved
extern bool RenderAtkSkillWindow;
extern bool RenderDocumentationWindow;

// ===== File Dialog Variables =====

extern std::string filepath;
extern std::string* multiselectpath;
extern int MultiSelectCount;

extern const COMDLG_FILTERSPEC skillfile[];
extern const COMDLG_FILTERSPEC skillpack[];

// ===== File I/O Variables & Shared Data =====

extern AttackSkill AtkSkill; // Attack skill struct
extern atkskill skillarray[751];

// ===== User Input Variables =====

extern char packname[32];
extern char PhantomDustDir[275];

// ========== Custom Functions ==========


std::string PWSTR_to_string(PWSTR ws);

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
