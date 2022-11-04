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

// The game's process ID.
DWORD pid = 0;

// A handle to the game process with read/write permissions.
HANDLE EsperHandle;

// 16 bytes of gstorage that can be checked against while searching memory to find where it begins.
static constexpr uint8_t gstorage_search[16] = { 0x04,0x40,0x04,0x00,0xA4,0xA7,0x01,0x00,0xF1,0x02,0x00,0x00,0xA4,0xA7,0x01,0x00 };

// Used to store the address where gstorage is located in the game's memory.
uintptr_t gstorage_address = 0;
gsdata gstorage = { 0 };

atkskill AtkSkill = { 0 };

DWORD get_pid_by_name(LPCTSTR ProcessName)
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

bool get_process()
{
    DWORD cache = pid;
    pid = get_pid_by_name("PDUWP.exe");
    EsperHandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!EsperHandle)
    {
        std::cout << "Failed to attach to process.\n";
        return false;
    }
    else if (cache != pid) // Find GSData in memory if it hasn't been scanned since the last reboot
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
                            if (memcmp(gstorage_search, &chunk[i], 16) == 0)
                            {
                                gstorage_address = (uintptr_t)p + i;
                                return true;
                            }
                        }
                    }
                }
                p += info.RegionSize;
            }
        }
    }

    return true;
}

bool load_gsdata_from_memory()
{
    if (EsperHandle != 0)
    {
        ReadProcessMemory(EsperHandle, (LPVOID)gstorage_address, &gstorage, sizeof(gstorage), NULL);
        DWORD error = GetLastError();
        if (error != 1400 && error != 183 && error != 0)
        {
            std::cout << "Process Read Error Code: " << error << "\n";
            return false;
        }
        return true;
    }
    else {
        return false;
    }
}

bool write_gsdata_to_memory()
{
    if (EsperHandle != 0)
    {
        WriteProcessMemory(EsperHandle, (LPVOID)gstorage_address, &gstorage, sizeof(gstorage), NULL);
        DWORD error = GetLastError();
        if (error != 1400 && error != 183 && error != 0)
        {
            std::cout << "Process Write Error Code: " << error << "\n";
            return false;
        }
        return true;
    }
    else {
        return false;
    }
}

void install_mod()
{
    if (load_gsdata_from_memory())
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
                gstorage.skill_array[(skill.SkillID - 1)] = skill; // Write skills from pack into gsdata (loaded in memory by LoadGSDATA())
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
        gstorage.VersionNum = (unsigned int)hash;
        std::cout << "New version number: " << hash << "\n";

        delete[] SkillPackBlobData;

        write_gsdata_to_memory();
    }
}

void pause_game()
{
    pid = get_pid_by_name("PDUWP.exe");
    DebugActiveProcess(pid);
}

void resume_game()
{
    pid = get_pid_by_name("PDUWP.exe");
    DebugActiveProcessStop(pid);
}