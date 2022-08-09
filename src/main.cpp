#include <algorithm>
#include <vector>

#include <main.h>
#include <UI.h>

extern "C" {
#include <crc32/crc_32.c>
}

using namespace std;

bool DebugMode = false;

// ===== File Dialog Variables =====

const COMDLG_FILTERSPEC skillfile[] = { L"Skill File", L"*.skill;" };
const COMDLG_FILTERSPEC skillpack[] = { L"Skill Pack", L"*.bin;" };
HRESULT hr;

char* filepathptr;
string filepath;

string* multiselectpath;
int MultiSelectCount = 0;

// ===== File I/O Variables & Shared Data =====

fstream AtkSkillFile; // fstream for Attack Skill files

fstream GSDataStream; // fstream for gsdata
GSDataHeader gsdataheader; // First 160 bytes of gsdata
atkskill skillarray[751];  // Array of 751 skill data blocks
AttackSkill AtkSkill;
int* gsdatamain; // Text, whitespace, other data shared among gsdata save/load functions

// ===== Process Editing Variables =====

DWORD pid = 0;
HANDLE EsperHandle;
uintptr_t baseAddress;
const uint8_t gsdata[24] = {0x04,0x40,0x04,0x00,0xA4,0xA7,0x01,0x00,0xF1,0x02,0x00,0x00,0xA4,0xA7,0x01,0x00,0x8C,0x00,0x00,0x00,0x76,0x01,0x00,0x00};
// ^ This causes a lot of warnings, should fix this later. (TODO)


// ===== User Input Variables =====

char packname[32];
char PhantomDustDir[275];


int main()
{
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    CreateUI(); // Main UI loop, see UI.h.
    CoUninitialize();
    return 0;
}


string PWSTR_to_string(PWSTR ws) {
    string result;
    result.reserve(wcslen(ws));
    for (; *ws; ws++)
        result += (char)*ws;
    return result;
}

// ===== File I/O =====

// Loads the current file into the AtkSkill struct
void LoadAttackSkill()
{
	AtkSkillFile.open(filepathptr, ios::in | ios::binary);   // Open file
	AtkSkillFile.read((char*)&AtkSkill, (sizeof(AtkSkill))); // Read bytes into AttackSkill struct

	AtkSkillFile.close();
	return;
}

// Writes the currently open file to disk.
void SaveAtkSkill()
{
    ofstream AtkSkillOut(filepathptr, ios::binary); // Creates a new ofstream variable, using
                                                    // the name of the file that was opened.

    AtkSkillOut.write((char*)&AtkSkill, 144);       // Overwrites the file that was opened with
                                                    // the new data.
    AtkSkillOut.close();

    skillarray[(AtkSkill.SkillID - 1)] = AtkSkill; // Write skills from pack into gsdata (loaded in memory by LoadGSDATA())

    if (GetProcess())
    {
        // Expensive calc, no point doing it unless we can access the version number
        uint32_t hash = crc32buf((char*)&AtkSkill, 144);
        gsdataheader.VersionNum = (int)hash;

        SaveGSDataToRAM();
        cout << "Wrote skill data to memory.\n";
        if (DebugMode)
        {
            cout << "New version number: " << hash << endl; // I want it to be slightly harder for new users to figure out
                                                            // how the hashing works, so hiding the version number change here.
        }
    }
}

void SaveSkillPack()
{
    ofstream SkillPackOut(filepathptr, ios::binary);
    fstream SkillStream; // fstream for the skill files selected by the user
    char skilldata[144]; // 144 byte buffer to read / write skill data from
    short FormatVersion = 1;
    short SkillCount = (short) MultiSelectCount; // Skill count == # of files selected
    int pad[3] = { 0,0,0 };

    SkillPackOut.write((char*)&packname, sizeof(packname)); // 32 characters for the name
    SkillPackOut.write((char*)&FormatVersion, sizeof(FormatVersion)); // This version is v1
    SkillPackOut.write((char*)&SkillCount, sizeof(SkillCount)); // So we know when to stop
    SkillPackOut.write((char*)&pad, 12); // Better alignment makes the file easier to read

    for (int i = 0; i < MultiSelectCount; i++)
    {
        if (filesystem::file_size(multiselectpath[i]) == 144) // Only allow skill data to be written if the skill file is the correct size
        {
            SkillStream.open(multiselectpath[i], ios::in | ios::binary);
            SkillStream.read((char*)&skilldata, 144); // Read skill data from the file to our skilldata buffer
            SkillStream.seekg(0);
            cout << "Writing from " << multiselectpath[i] << "...\n";
            SkillPackOut.write((char*)&skilldata, 144); // Writing to skill pack
        }
        else
        {
            cout << "Invalid skill filesize. Write skipped.\n";
        }
    }
    SkillStream.close();

    SkillPackOut.close();
    cout << "Saved skill pack to " << filepathptr << "\n";
}

// void InstallSkillPack()
// {
//     if (LoadGSDATA() == 0)
//     {
//         int* Filesize;
//         Filesize = new int[MultiSelectCount];
//         fstream SkillPackBlob; // Separate stream that will only have skill pack data, so that we can just pass it as a buffer to be hashed.
//                                // This is way more efficient than writing them all to a single file on disk, hashing that, then deleting it.
//         int BlobSize = 0;
//         for (unsigned int n = 0; n < MultiSelectCount; n++) // Loop through every selected skill pack file
//         {
//             fstream SkillPackIn;
//             SkillPackHeaderV1 header;
//             SkillPackIn.open(multiselectpath[n], ios::in | ios::binary);
//             SkillPackIn.read((char*)&header, sizeof(header));
//             Filesize[n] = (int) std::filesystem::file_size(multiselectpath[n]);
// 
//             for (int i = 0; i < header.SkillCount; i++)
//             {
//                 atkskill skill; // Instance of struct. ID will be in the same posiiton every time, so it's fine to use the attack template.
//                 SkillPackIn.read((char*)&skill, sizeof(skill)); // Read a skill into struct
//                 skillarray[(skill.SkillID - 1)] = skill; // Write skills from pack into gsdata (loaded in memory by LoadGSDATA())
//             }
//         }
//         for (unsigned int n = 0; n < MultiSelectCount; n++)
//         {
//             BlobSize += Filesize[n];
//         }
//         SkillPackBlobData = new char[BlobSize];
//         for (unsigned int n = 0; n < MultiSelectCount; n++)
//         {
//             SkillPackBlob.open(multiselectpath[n], ios::in | ios::binary);
//             SkillPackBlob.read(SkillPackBlobData, BlobSize);
//         }
//         SkillPackBlob.close();
// 
//         uint32_t hash = crc32buf(SkillPackBlobData, BlobSize);
//         gsdataheader.VersionNum = (int) hash;
//         cout << hash << "\n";
// 
//         SaveGSDATA();
//     }
// }
// 
// int LoadGSDATA()
// {
//     filesystem::path gsdatapath{ (PhantomDustDir + (string)"\\Assets\\Data\\gstorage\\gsdata_en.dat") };
//     if (!filesystem::exists(gsdatapath))
//     {
//         cout << "Invalid Phantom Dust folder.\n";
//         return 1; // Failure
//     }
// 
//     GSDataStream.open(gsdatapath, ios::in | ios::binary); // Open file
//     GSDataStream.read((char*)&gsdataheader, (sizeof(gsdataheader))); // Read bytes into gsdataheader struct
// 
//     GSDataStream.read((char*)skillarray, (sizeof(skillarray)));
// 
//     int datasize = (sizeof(gsdataheader) + sizeof(skillarray)) / 4;
// 
//     gsdatamain = new int[((gsdataheader.Filesize - datasize))];
//     GSDataStream.read((char*)gsdatamain, (datasize * 4)); // Multiplied by 4 because each int is 4 bytes
// 
//     GSDataStream.close();
//     return 0; // Success
// }
// 
// int SaveGSDATA()
// {
//     filesystem::path gsdatapath{ (PhantomDustDir + (string)"\\Assets\\Data\\gstorage\\gsdata_en.dat") };
//     if (!filesystem::exists(gsdatapath))
//     {
//         cout << "Invalid Phantom Dust folder.\n";
//         return 1; // Failure
//     }
//     ofstream GSDataOut(gsdatapath, ios::binary); // Creates a new ofstream variable, using
//                                                  // the name of the file that was opened.
// 
//     GSDataOut.write((char*)&gsdataheader, 160);  // Overwrites the file that was opened with
// 
//     GSDataOut.write((char*)&skillarray, sizeof(skillarray));  // Overwrites the file that was opened with
// 
//     GSDataOut.write((char*)gsdatamain, (gsdataheader.Filesize - 108304));
//     GSDataOut.close();
//     return 0; // Success
// }

// ===== Process Editing Functions =====

DWORD GetProcessIDByName(LPCTSTR ProcessName)
{
    PROCESSENTRY32 pt;
    HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    pt.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hsnap, &pt)) { // must call this first
        do {
            if (!lstrcmpi(pt.szExeFile, ProcessName)) {
                CloseHandle(hsnap);
                return pt.th32ProcessID;
            }
        } while (Process32Next(hsnap, &pt));
    }
    CloseHandle(hsnap); // close handle on failure
    return 0;
}

char* GetAddressOfData(const uint8_t* data, size_t len)
{
    if (EsperHandle)
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);

        MEMORY_BASIC_INFORMATION info;
        std::vector<char> chunk;
        char* p = (char*)0x7FF600000000;
        while (p < si.lpMaximumApplicationAddress)
        {
            if (VirtualQueryEx(EsperHandle, p, &info, sizeof(info)) == sizeof(info))
            {
                if (info.State == MEM_COMMIT)   // If this is a valid chunk of memory
                {
                    chunk.resize(info.RegionSize);
                    SIZE_T bytesRead;
                    if (ReadProcessMemory(EsperHandle, p, &chunk[0], info.RegionSize, &bytesRead))
                    {
                        for (size_t i = 0; i < (bytesRead - len); ++i)
                        {
                            if (memcmp(data, &chunk[i], len) == 0)
                            {
                                return (char*)p + i;
                            }
                        }
                    }
                }
                p += info.RegionSize;
            }
        }
    }
    return 0;
}

bool GetProcess()
{
    DWORD cache = pid;
    pid = GetProcessIDByName(L"PDUWP.exe");
    EsperHandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!EsperHandle)
    {
        cout << "Failed to attach to process.\n";
        return false;
    }
    if (cache != pid) // Find GSData in memory if it hasn't been scanned since the last reboot
    {
        baseAddress = (uintptr_t)GetAddressOfData(gsdata, 24);
    }

    return true;
 }

int LoadGSDataFromRAM()
{
    if (EsperHandle != 0)
    {
        ReadProcessMemory(EsperHandle, (LPVOID)baseAddress, &gsdataheader, sizeof(gsdataheader), NULL);
        if (GetLastError() != 1400 && GetLastError() != 183 && GetLastError() != 0)
        {
            cout << "Process Read Error Code: " << GetLastError() << endl;
        }
        baseAddress += 160; // Address where the skills begin.
        ReadProcessMemory(EsperHandle, (LPVOID)baseAddress, &skillarray, sizeof(skillarray), NULL);
        if (GetLastError() != 1400 && GetLastError() != 183 && GetLastError() != 0)
        {
            cout << "Process Read Error Code: " << GetLastError() << endl;
        }
        baseAddress -= 160;
    }
    return 0;
}

int SaveGSDataToRAM()
{
    if (EsperHandle != 0)
    {
        WriteProcessMemory(EsperHandle, (LPVOID)baseAddress, &gsdataheader, sizeof(gsdataheader), NULL);
        if (GetLastError() != 1400 && GetLastError() != 183 && GetLastError() != 0)
        {
            cout << "Process Write Error Code: " << GetLastError() << endl;
        }
        baseAddress += 160; // Address where the skills begin.
        WriteProcessMemory(EsperHandle, (LPVOID)baseAddress, &skillarray, sizeof(skillarray), NULL);
        if (GetLastError() != 1400 && GetLastError() != 183 && GetLastError() != 0)
        {
            cout << "Process Write Error Code: " << GetLastError() << endl;
        }
        baseAddress -= 160;
    }
    return 0;
}

void InstallSkillPackToRAM()
{
    if (LoadGSDataFromRAM() == 0)
    {
        std::vector<string> strArray(multiselectpath, multiselectpath + MultiSelectCount);
        std::sort(strArray.begin(), strArray.end()); // Sort paths alphabetically

        int BlobSize = 0;
        for (int n = 0; n < MultiSelectCount; n++) // Loop through every selected skill pack file
        {
            fstream SkillPackIn;
            SkillPackHeaderV1 header;
            SkillPackIn.open(multiselectpath[n], ios::in | ios::binary);
            SkillPackIn.read((char*)&header, sizeof(header));
            BlobSize += (int)std::filesystem::file_size(multiselectpath[n]);

            for (int i = 0; i < header.SkillCount; i++)
            {
                atkskill skill; // Instance of struct. ID will be in the same posiiton every time, so it's fine to use the attack template.
                SkillPackIn.read((char*)&skill, sizeof(skill)); // Read a skill into struct
                skillarray[(skill.SkillID - 1)] = skill; // Write skills from pack into gsdata (loaded in memory by LoadGSDATA())
            }
        }
        char* SkillPackBlobData;
        SkillPackBlobData = new char[BlobSize];
        fstream SkillPackBlob; // Separate stream that will only have skill pack data, so that we can just pass it as a buffer to be hashed.
                               // This is way more efficient than writing them all to a single file on disk, hashing that, then deleting it.
        for (int n = 0; n < MultiSelectCount; n++)
        {
            SkillPackBlob.open(multiselectpath[n], ios::in | ios::binary);
            SkillPackBlob.read(SkillPackBlobData, BlobSize);
        }
        SkillPackBlob.close();

        uint32_t hash = crc32buf(SkillPackBlobData, BlobSize);
        gsdataheader.VersionNum = (int)hash;
        cout << "New version number: " << hash << endl;

        delete[] SkillPackBlobData;

        SaveGSDataToRAM();
    }
}

void PauseGame()
{
    pid = GetProcessIDByName(L"PDUWP.exe");
    DebugActiveProcess(pid);
}

void UnpauseGame()
{
    pid = GetProcessIDByName(L"PDUWP.exe");
    DebugActiveProcessStop(pid);
}

// ===== Custom ImGui Functions / Wrappers =====

void Tooltip(const char* text)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", text);
    }
}

// ImGui didn't have pre-made functions for short
// or uint8 input / combo boxes, so I made my own.

const short s8_one = 1;
const short s16_one = 1;
void InputShort(const char* label, void* p_data)
{
    ImGui::InputScalar(label, ImGuiDataType_S16, p_data, true ? &s16_one : NULL, NULL, "%d");
}

void InputUInt8(const char* label, void* p_data)
{
    ImGui::InputScalar(label, ImGuiDataType_S8, p_data, true ? &s8_one : NULL, NULL, "%d");
}

// ===== Windows File Dialogs =====

int WINAPI FileSelectDialog(const COMDLG_FILTERSPEC *fileTypes)
{
    IFileOpenDialog* pFileOpen;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr))
    {
        // Show the Open dialog box.
        hr = pFileOpen->SetFileTypes(2, fileTypes);
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

HRESULT MultiSelectWindow()
{
    IFileOpenDialog* pfd;

    // CoCreate the dialog object.
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));

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
            hr = pfd->SetDefaultExtension(L".bin");
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

int WINAPI FileSaveDialog(const COMDLG_FILTERSPEC *fileTypes, LPCWSTR DefaultExtension)
{
    IFileSaveDialog* pFileOpen;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr))
    {
        // Show the Open dialog box.
        hr = pFileOpen->SetFileTypes(IM_ARRAYSIZE(skillpack), fileTypes);
        hr = pFileOpen->SetDefaultExtension(DefaultExtension);
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
