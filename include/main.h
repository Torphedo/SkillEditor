#pragma once

#include <stdint.h>
#include <string>
#include <imgui_internal.h>
#include <structs.h>


extern "C" {
#include <crc32/crc_32.c>
}

using namespace std;

// ===== File Dialog Variables =====

const COMDLG_FILTERSPEC fileTypes[] = { L"Skill File", L"*.skill;" };
HRESULT hr;
string filepath;
char* filepathptr;
string* multiselectpath;
DWORD MultiSelectCount;

// ===== File I/O Variables & Shared Data =====

fstream AtkSkillFile; // fstream for Attack Skill files
fstream GSDataStream; // fstream for gsdata
GSDataHeader gsdataheader; // First 160 bytes of gsdata
atkskill skillarray[751];  // Array of 751 skill data blocks
int* gsdatamain; // Text, whitespace, other data shared among gsdata save/load functions

// ===== User Input Variables =====

char packname[32];
char PhantomDustDir[275];

// ===== UI Variables =====
bool OptionsWindow = false;
bool SkillPackWindow = false;


// ========== Custom Functions ==========


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
void SaveSkillPack();
void LoadGSDATA();
void SaveGSDATA();

// ===== Custom ImGui Functions / Wrappers =====

void Tooltip(const char* text);
void InputShort(const char* label, void* p_data);
void InputUInt8(const char* label, void* p_data);
short ComboShort(const char* label, const char* const* items, int item_count);

// ===== Windows Explorer Dialogs =====

int WINAPI FileSelectDialog()
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
    return 0;
}

HRESULT MultiselectInvoke()
{
    IFileOpenDialog* pfd;
    
    // CoCreate the dialog object.
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    
    if (SUCCEEDED(hr))
    {
        DWORD dwOptions;
        // Specify multiselect.
        hr = pfd->GetOptions(&dwOptions);
    
        if (SUCCEEDED(hr))
        {
            hr = pfd->SetOptions(dwOptions | FOS_ALLOWMULTISELECT);
        }
    
        if (SUCCEEDED(hr))
        {
            // Show the Open dialog.
            hr = pfd->Show(NULL);
    
            if (SUCCEEDED(hr))
            {
                // Obtain the result of the user interaction.
                IShellItemArray* psiaResults;
                hr = pfd->GetResults(&psiaResults);
    
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath = NULL;
                    DWORD dwNumItems = 0; // number of items in multiple selection
                    std::wstring strSelected; // will hold file paths of selected items
    
                    hr = psiaResults->GetCount(&dwNumItems);  // get number of selected items
    
                    if (SUCCEEDED(hr))
                    {
                        multiselectpath = new string[dwNumItems];
                        // Loop through IShellItemArray and construct string for display
                        for (DWORD i = 0; i < dwNumItems; i++)
                        {
                            IShellItem* psi = NULL;
                            MultiSelectCount = dwNumItems;
    
                            hr = psiaResults->GetItemAt(i, &psi); // get a selected item from the IShellItemArray
    
                            if (SUCCEEDED(hr))
                            {
                                hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    
                                if (SUCCEEDED(hr))
                                {
                                    multiselectpath[i] = PWSTR_to_string(pszFilePath);
    
                                    CoTaskMemFree(pszFilePath);
                                }
    
                                psi->Release();
                            }
                        }
                    }
                }
            }
        }
        pfd->Release();
    }
    return hr;
}

int WINAPI FileSaveDialog()
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
    return 0;
}
