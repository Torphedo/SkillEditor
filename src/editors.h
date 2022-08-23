#pragma once

void SafeNewPack();
void AtkSkillWindow();
void DocumentationWindow();
// We can use a short here because the index will be 0 - 750
void HexEditorWindow(short Idx);
void SkillPackWindow();

static const char* DocumentationAtkBody[50] = {
#include "../res/AttackSkillBody.txt"
};

static const char* DocumentationAtkLabels[50] = {
#include "../res/AttackSkillLabels.txt"
};
