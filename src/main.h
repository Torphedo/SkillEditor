#pragma once

#include <shobjidl.h> 

extern bool DebugMode;

// ===== UI Variables =====

typedef struct
{
	short ErrorCode;
	bool NewSkillPack;
	bool HexEditor;
	bool AttackSkillEditor;
	bool Documentation;
	bool IDSelection;
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
