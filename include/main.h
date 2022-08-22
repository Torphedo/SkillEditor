#pragma once

#include <Windows.h>
#include <shobjidl.h> 
#include <stdint.h>
#include <string>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>

#include <imgui_internal.h>
#include <structs.h>

extern bool DebugMode;

// ===== UI Variables =====

typedef struct
{
	short ErrorCode;
	bool NewSkillPack;
	bool HexEditor;
	bool AttackSkillEditor;
	bool Documentation;
}windowvars; extern windowvars UI;

extern bool OpenedAttackSkill;

extern const COMDLG_FILTERSPEC skillfile[];
extern const COMDLG_FILTERSPEC skillpack[];

// ===== User Input Variables =====

extern char packname[32];

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
