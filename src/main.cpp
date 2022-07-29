#include <Windows.h>
#include <shobjidl.h> 
#include <string>
#include <iostream>
#include <fstream>

#include <main.h>
#include <setupUI.h>
using namespace std;

int main()
{
    CreateUI();
}

void LoadAttackSkill(char* filename)
{
	AtkSkillFile.open(filename, ios::in | ios::binary); // Open file
	AtkSkillFile.read((char*)&AtkSkill, (sizeof(AtkSkill)));    // Read bytes into AttackSkill struct

	AtkSkillFile.close();
	return;
}
