#pragma once

#include <shobjidl.h>
#include <string>

extern std::string* multiselectpath;
extern int MultiSelectCount;

void init_winapi();
HRESULT file_multiple_select_dialog();
char* WINAPI file_select_dialog(const COMDLG_FILTERSPEC fileTypes);
char* WINAPI file_save_dialog(const COMDLG_FILTERSPEC fileTypes, LPCWSTR DefaultExtension);
