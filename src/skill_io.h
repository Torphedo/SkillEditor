#pragma once
#include "structs.h"

atkskill load_attack_skill();
void save_attack_skill(atkskill skill);
void save_skill_pack(const char* packname);
void save_attack_skill_with_file_select(atkskill skill);
