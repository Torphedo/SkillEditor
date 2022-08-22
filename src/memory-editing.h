#pragma once
#include "structs.h"

extern std::fstream AtkSkillFile;
extern GSDataHeader gsdataheader;
extern atkskill skillarray[751];
extern AttackSkill AtkSkill; // Attack skill struct

DWORD GetProcessIDByName(LPCTSTR ProcessName);
bool GetProcess();
int LoadGSDataFromRAM();
int SaveGSDataToRAM();
void InstallSkillPackToRAM();
void PauseGame();
void UnpauseGame();