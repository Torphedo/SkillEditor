#include <iostream>
#include <filesystem>
#include <fstream>
#include <Windows.h>
#include <shobjidl.h> 
#include <tlhelp32.h>

#include "memory-editing.h"
#include "structs.h"
#include "winAPI.h"

extern "C" {
#include <crc_32.h>
}

DWORD pid = 0;
HANDLE EsperHandle;
uintptr_t baseAddress;
constexpr static uint8_t gsdata[16] = { 0x04,0x40,0x04,0x00,0xA4,0xA7,0x01,0x00,0xF1,0x02,0x00,0x00,0xA4,0xA7,0x01,0x00 };

std::fstream AtkSkillFile; // fstream for Attack Skill files

GSDataHeader gsdataheader; // First 160 bytes of gsdata
atkskill skillarray[751];  // Array of 751 skill data blocks
AttackSkill AtkSkill;
int* gsdatamain; // Text, whitespace, other data shared among gsdata save/load functions

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

bool GetProcess()
{
    DWORD cache = pid;
    pid = GetProcessIDByName(L"PDUWP.exe");
    EsperHandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!EsperHandle)
    {
        std::cout << "Failed to attach to process.\n";
        return false;
    }
    if (cache != pid) // Find GSData in memory if it hasn't been scanned since the last reboot
    {
        if (EsperHandle)
        {
            SYSTEM_INFO si;
            GetSystemInfo(&si);

            MEMORY_BASIC_INFORMATION info;
            std::vector<char> chunk;
            char* p = (char*)0x7FF600000000; // Address to start searching for valid memory pages
            while (p < si.lpMaximumApplicationAddress)
            {
                // Loop through memory pages to find one that's actually being used by the game
                if (VirtualQueryEx(EsperHandle, p, &info, sizeof(info)) == sizeof(info))
                {
                    if (info.State == MEM_COMMIT) // If this is a valid page of memory
                    {
                        chunk.resize(info.RegionSize);
                        SIZE_T bytesRead;
                        if (ReadProcessMemory(EsperHandle, p, &chunk[0], info.RegionSize, &bytesRead))
                        {
                            for (size_t i = 0; i < (bytesRead - 16); ++i)
                            {
                                // Check if the 16 bytes at the current address matches gsdata
                                if (memcmp(gsdata, &chunk[i], 16) == 0)
                                {
                                    baseAddress = (uintptr_t)p + i;
                                    LoadGSDataFromRAM();
                                    return true;
                                }
                            }
                        }
                    }
                    p += info.RegionSize;
                }
            }
        }
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
            std::cout << "Process Read Error Code: " << GetLastError() << "\n";
        }
        baseAddress += 160; // Address where the skills begin.
        ReadProcessMemory(EsperHandle, (LPVOID)baseAddress, &skillarray, 751 * 144, NULL);
        if (GetLastError() != 1400 && GetLastError() != 183 && GetLastError() != 0)
        {
            std::cout << "Process Read Error Code: " << GetLastError() << "\n";
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
            std::cout << "Process Write Error Code: " << GetLastError() << "\n";
        }
        baseAddress += 160; // Address where the skills begin.
        WriteProcessMemory(EsperHandle, (LPVOID)baseAddress, &skillarray, sizeof(skillarray), NULL);
        if (GetLastError() != 1400 && GetLastError() != 183 && GetLastError() != 0)
        {
            std::cout << "Process Write Error Code: " << GetLastError() << "\n";
        }
        baseAddress -= 160;
    }
    return 0;
}

void InstallSkillPackToRAM()
{
    if (LoadGSDataFromRAM() == 0)
    {
        std::vector<std::string> strArray(multiselectpath, multiselectpath + MultiSelectCount);
        std::sort(strArray.begin(), strArray.end()); // Sort paths alphabetically

        int BlobSize = 0;
        for (int n = 0; n < MultiSelectCount; n++) // Loop through every selected skill pack file
        {
            SkillPackHeaderV1 header;
            FILE* SkillPack;
            fopen_s(&SkillPack, multiselectpath[n].c_str(), "rb");
            if (SkillPack == 0)
            {
                std::cout << "Failed to load file!";
                return;
            }
            fread_s(&header, sizeof(header), sizeof(header), 1, SkillPack);
            BlobSize += (int)std::filesystem::file_size(multiselectpath[n]);

            for (int i = 0; i < header.SkillCount; i++)
            {
                atkskill skill; // Instance of struct. ID will be in the same posiiton every time, so it's fine to use the attack template.
                fread_s(&skill, sizeof(skill), sizeof(skill), 1, SkillPack);
                skillarray[(skill.SkillID - 1)] = skill; // Write skills from pack into gsdata (loaded in memory by LoadGSDATA())
            }
            fclose(SkillPack);
        }
        char* SkillPackBlobData = new char[BlobSize];
        FILE* SkillPackBlob; // Separate stream that will only have skill pack data, so that we can just pass it as a buffer to be hashed.
                             // This is way more efficient than writing them all to a single file on disk, hashing that, then deleting it.

        for (int n = 0; n < MultiSelectCount; n++)
        {
            fopen_s(&SkillPackBlob, multiselectpath[n].c_str(), "rb");
            if (SkillPackBlob == 0)
            {
                std::cout << "Failed to load file!";
                fclose(SkillPackBlob);
                return;
            }
            fread_s(SkillPackBlobData, sizeof(SkillPackBlobData), sizeof(SkillPackBlobData), 1, SkillPackBlob);
        }
        fclose(SkillPackBlob);

        uint32_t hash = crc32buf(SkillPackBlobData, BlobSize);
        gsdataheader.VersionNum = (unsigned int)hash;
        std::cout << "New version number: " << hash << "\n";

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