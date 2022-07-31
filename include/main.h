#pragma once

#include <stdint.h>
#include <string>
#include <imgui_internal.h>
#include <structs.h>

using namespace std;

fstream AtkSkillFile;
bool OptionsWindow = false;
static char PhantomDustDir[275];

string filepath;
char* filepathptr;
const COMDLG_FILTERSPEC fileTypes[] = { L"Skill File", L"*.skill;" }; // For file dialogs

string PWSTR_to_string(PWSTR ws) {
    string result;
    result.reserve(wcslen(ws));
    for (; *ws; ws++)
        result += (char)*ws;
    return result;
}

// ===== File I/O =====

void LoadAttackSkill();
void SaveAtkSkill(); 

// ===== Custom ImGui Functions / Wrappers =====

void Tooltip(const char* text);
void InputShort(const char* label, void* p_data);
void InputUInt8(const char* label, void* p_data);
short ComboShort(const char* label, const char* const* items, int item_count);

// ===== Windows Explorer Dialogs =====

int WINAPI FileSelectDialog()
{
    // Confusing / overcomplicated way of opening a file dialog
    // taken from Microsoft's documentation. There's probably a
    // shorter way to do this.
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (hr == 0x800704c7) // ERROR_CANCELLED
            {
                return -1;
            }
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        filepath = PWSTR_to_string(pszFilePath);
                        filepathptr = const_cast<char*>(filepath.c_str());
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return 0;
}

int WINAPI FileSaveDialog()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileSaveDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);
            hr = pFileOpen->SetDefaultExtension(L".skill");
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (hr == 0x800704c7) // ERROR_CANCELLED
            {
                return -1;
            }
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        filepath = PWSTR_to_string(pszFilePath);
                        filepathptr = const_cast<char*>(filepath.c_str());
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return 0;
}
