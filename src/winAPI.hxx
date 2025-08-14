#pragma once

#include <shobjidl.h>
#include <string>

extern std::string* multiselectpath;
extern unsigned int MultiSelectCount;

void init_winapi();
HRESULT file_multiple_select_dialog();