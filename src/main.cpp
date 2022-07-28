#include <Windows.h>
#include <shobjidl.h> 
#include <string>
#include <iostream>
#include <fstream>

#include <main.h>
#include <setupUI.h>
using namespace std;

const COMDLG_FILTERSPEC fileTypes[] =
{
    { L"Skill File", L"*.skill;" },
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
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


int main()
{
    CreateUI();
}

void LoadAttackSkill(char* filename)
{
	fstream AtkSkillFile;
	AttackSkill AtkSkill;

	AtkSkillFile.open(filename, ios::in | ios::binary); // Open file
	AtkSkillFile.read((char*)&AtkSkill, (sizeof(AtkSkill)));    // Read bytes into AttackSkill struct

	AtkSkillFile.close();
	return;
}
