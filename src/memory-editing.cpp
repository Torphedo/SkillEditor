#include <iostream>
#include <algorithm>
#include <vector>
#include <filesystem>
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

static DWORD get_pid_by_name(LPCTSTR ProcessName)
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

DWORD process_id()
{
    return pid;
}

bool is_running()
{
    pid = get_pid_by_name("PDUWP.exe");
    return (pid != 0);
}

bool can_read_memory()
{
    static unsigned char buf = 0;
    ReadProcessMemory(EsperHandle, (LPVOID)gstorage_address, &buf, 1, NULL);
    DWORD error = GetLastError();
    bool test = (error == 0);
    return (error == 0);
}

bool have_process_handle()
{
    bool test = (EsperHandle != 0);
    return (EsperHandle != 0);
}

bool get_process()
{
    if (is_running())
    {
        if ((!have_process_handle() || !can_read_memory()))
        {
            EsperHandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);
            if (EsperHandle != 0)
            {
                printf("Got handle: %p\n", EsperHandle);

                SYSTEM_INFO sys;
                GetSystemInfo(&sys);
                MEMORY_BASIC_INFORMATION info;
                std::vector<char> chunk;
                char* p = (char*)0x7FF600000000; // Address to start searching for valid memory pages
                while (p < sys.lpMaximumApplicationAddress)
                {
                    // Loop through memory pages to find one being used by the game
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
                return false; // Triggered if gsdata is never found
            }
            else {
                printf("Failed to open Phantom Dust process with process ID %li.\n", pid);
                return false;
            }
        }
        else { return true; }
    }
    else { return false; }

}

bool load_skill_data()
{
    if (EsperHandle != 0)
    {
        DWORD error = 0;

        ReadProcessMemory(EsperHandle, (LPVOID)gstorage_address, &gstorage, sizeof(gstorage), NULL);
        error = GetLastError();
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

skill_text load_skill_text(unsigned int id)
{
    if (EsperHandle != 0)
    {
        // Memory address of the skill text table & strings
        // static uintptr_t text_section = gstorage_address + (gstorage.unk0 & 0xFFFFF000) + 0x1A000;
        static uintptr_t text_section = gstorage_address + 0x34000;

        // Read section header to check the number of strings
        text_header header = { 0 };
        ReadProcessMemory(EsperHandle, (LPVOID)text_section, &header, sizeof(text_header), NULL);
        if (GetLastError() != 0)
        {
            printf("Failed to read process memory (code %li)\n", GetLastError());
            return { "Failed to read data.", "Failed to read data." };
        }

        if (id > header.array_size - 1)
        {
            return { "No text found.", "No text found." };
        }

        uintptr_t text_ptr_location = text_section + sizeof(text_header) + ((id - 1) * 0xC);

        text_ptrs string_offset[2] = {0};
        ReadProcessMemory(EsperHandle, (LPVOID)text_ptr_location, &string_offset, sizeof(text_ptrs) * 2, NULL);
        if (GetLastError() != 0)
        {
            printf("Failed to read process memory (code %li)\n", GetLastError());
            return { "Failed to read data.", "Failed to read data." };
        }

        uint32_t name_size = string_offset[0].desc - string_offset[0].name;
        uint32_t desc_size = string_offset[1].name - string_offset[0].desc + sizeof(text_ptrs);
        char* name = (char*)calloc(1, name_size);
        char* desc = (char*)calloc(1, desc_size);

        ReadProcessMemory(EsperHandle, (LPVOID)(text_ptr_location + string_offset[0].name), name, name_size, NULL);
        if (GetLastError() != 0)
        {
            printf("Failed to read process memory (code %li)\n", GetLastError());
            return { "Failed to read data.", "Failed to read data." };
        }
        ReadProcessMemory(EsperHandle, (LPVOID)(text_ptr_location + string_offset[0].desc), desc, desc_size, NULL);
        if (GetLastError() != 0)
        {
            printf("Failed to read process memory (code %li)\n", GetLastError());
            return { "Failed to read data.", "Failed to read data." };
        }

        skill_text output = { name, desc };
        free(name);
        free(desc);

        return output;
    }
    else {
        return { "No running PD instance.", "No running PD instance." };
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
    if (load_skill_data())
    {
        std::vector<std::string> strArray(multiselectpath, multiselectpath + MultiSelectCount);
        std::sort(strArray.begin(), strArray.end()); // Sort paths alphabetically

        int BlobSize = 0;
        for (int n = 0; n < MultiSelectCount; n++) // Loop through every selected skill pack file
        {
            packheader1 header;
            FILE* SkillPack;
            fopen_s(&SkillPack, multiselectpath[n].c_str(), "rb");
            if (SkillPack == 0)
            {
                std::cout << "Failed to load file!";
                return;
            }
            fread_s(&header, sizeof(header), sizeof(header), 1, SkillPack);
            BlobSize += (int)std::filesystem::file_size(multiselectpath[n]);

            atkskill* skills = new atkskill[header.SkillCount];
            for (int i = 0; i < header.SkillCount; i++)
            {
                fread(&skills[i], sizeof(atkskill), 1, SkillPack);
                gstorage.skill_array[(skills[i].SkillID - 1)] = skills[i]; // Write skills from pack into gsdata
            }
            delete[] skills;
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