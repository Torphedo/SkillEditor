#pragma once

#include <commands.h>
#include <cstdint>

void load_skills();

void unlock_all_skills();

int PLUGIN_API command_load_skill(int argc, char** argv);
