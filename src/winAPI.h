#pragma once

#include <Windows.h>
#include <shobjidl.h>
#include <string>

extern char* filepath;
extern std::string* multiselectpath;
extern int MultiSelectCount;

extern HRESULT hr;

HRESULT MultiSelectWindow();
int WINAPI FileSelectDialog(const COMDLG_FILTERSPEC fileTypes);
int WINAPI FileSaveDialog(const COMDLG_FILTERSPEC fileTypes, LPCWSTR DefaultExtension);
