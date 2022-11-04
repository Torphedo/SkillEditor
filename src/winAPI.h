#pragma once

#include <Windows.h>
#include <shobjidl.h>
#include <string>

extern std::string* multiselectpath;
extern int MultiSelectCount;

extern HRESULT hr;

HRESULT file_multiple_select_dialog();
char* WINAPI file_select_dialog(const COMDLG_FILTERSPEC fileTypes);
char* WINAPI file_save_dialog(const COMDLG_FILTERSPEC fileTypes, LPCWSTR DefaultExtension);
