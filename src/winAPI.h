#pragma once

#include <Windows.h>
#include <shobjidl.h>

const COMDLG_FILTERSPEC skillfile[] = { L"Skill File", L"*.skill;" };
const COMDLG_FILTERSPEC skillpack[] = { L"Skill Pack", L"*.bin;" };
extern HRESULT hr;

HRESULT MultiSelectWindow();
int WINAPI FileSelectDialog(const COMDLG_FILTERSPEC* fileTypes);
int WINAPI FileSaveDialog(const COMDLG_FILTERSPEC* fileTypes, LPCWSTR DefaultExtension);
