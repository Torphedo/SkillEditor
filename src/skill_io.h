#pragma once
#include "structs.h"
#include <fstream>

AttackSkill load_attack_skill();
void save_attack_skill();
void save_skill_pack(const char* packname);

void save_attack_skill_with_file_select();
