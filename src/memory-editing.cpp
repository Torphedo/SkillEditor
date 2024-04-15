#include <Windows.h>
#include <shobjidl.h> 
#include <tlhelp32.h>
#include <psapi.h>

#include "memory-editing.h"
#include "structs.h"
#include "winAPI.h"

extern "C" {
#include <crc_32.h>
}

static DWORD get_pid_by_name(LPCTSTR ProcessName) {
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

u32 is_running() {
    return get_pid_by_name("PDUWP.exe");
}

bool can_read_memory(pd_meta p) {
    unsigned char buf = 0;
    ReadProcessMemory(p.h, (LPVOID)p.gstorage_addr, &buf, 1, NULL);
    DWORD error = GetLastError();
    SetLastError(0);
    // Ignore error 298
    // ("too many posts made to semaphore", caused when we spam remote memory reads)
    return (error == 298) || (error = 0);
}

bool have_process_handle(pd_meta p) {
    return (p.h != INVALID_HANDLE_VALUE);
}

pd_meta get_process() {
    pd_meta out = {0};
    if (!is_running()) {
        return out;
    }
    out.pid = get_pid_by_name("PDUWP.exe");

    // Open game process
    DWORD access = PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION;
    out.h = OpenProcess(access, FALSE, out.pid);
    if (out.h == INVALID_HANDLE_VALUE) {
        return out;
    }
    printf("Got handle: %p\n", out.h);

    HMODULE modules[1024] = {0};
    DWORD bytes_needed = 0;

    // Get path to PDUWP.exe
    char base_exe_name[MAX_PATH] = {0};
    HMODULE base_exe_module = 0;
    int size = sizeof(base_exe_name) / sizeof(*base_exe_name);
    GetModuleFileNameEx(out.h, NULL, base_exe_name, sizeof(base_exe_name) / sizeof(TCHAR));

    if (EnumProcessModules(out.h, modules, sizeof(modules), &bytes_needed)) {
        for (uint32_t i = 0; i < (bytes_needed / sizeof(HMODULE)); i++) {
            char module_name[MAX_PATH] = {0};

            if (GetModuleFileNameExA(out.h, modules[i], module_name, sizeof(module_name) / sizeof(TCHAR))) {
                // Check name against the base EXE name (PDUWP.exe)
                if (strncmp(module_name, base_exe_name, MAX_PATH) == EXIT_SUCCESS) {
                    base_exe_module = modules[i];
                    break;
                }
            }
        }
    }

    if (base_exe_module == INVALID_HANDLE_VALUE) {
        printf("Couldn't find PDUWP.exe base address.\n");
        return out;
    }

    out.gstorage = (gsdata*)malloc(sizeof(*out.gstorage));
    out.gstorage_addr = ((uintptr_t)base_exe_module + gstorage_offset);

    ReadProcessMemory(out.h, (LPVOID)out.gstorage_addr, out.gstorage, sizeof(*out.gstorage), NULL);
    DWORD error = GetLastError();
    if (error != 0) {
        printf("Process Read Error Code: %ld\n", error);
    }

    return out; // We couldn't get the gsdata offset.
}

// TODO: This is some janky bullshit.
skill_text load_skill_text(pd_meta p, unsigned int id) {
    if (p.h != INVALID_HANDLE_VALUE) {
        SetLastError(0);
        // Memory address of the skill text table & strings
        // static uintptr_t text_section = gstorage_address + (gstorage.unk0 & 0xFFFFF000) + 0x1A000;
        static uintptr_t text_section = p.gstorage_addr + 0x34000;

        // Read section header to check the number of strings
        text_header header = { 0 };
        ReadProcessMemory(p.h, (LPVOID)text_section, &header, sizeof(text_header), NULL);
        if (GetLastError() != 0) {
            printf("Failed to read process memory (code %li)\n", GetLastError());
            return { "", ""};
        }

        if (id > header.array_size - 1) {
            return { "No text found.", "No text found." };
        }

        uintptr_t text_ptr_location = text_section + sizeof(text_header) + ((id - 1) * 0xC);

        text_ptrs string_offset[2] = {0};
        ReadProcessMemory(p.h, (LPVOID)text_ptr_location, &string_offset, sizeof(text_ptrs) * 2, NULL);
        if (GetLastError() != 0) {
            printf("Failed to read process memory (code %li)\n", GetLastError());
            return { "", "" };
        }

        uint32_t name_size = string_offset[0].desc - string_offset[0].name;
        uint32_t desc_size = string_offset[1].name - string_offset[0].desc + sizeof(text_ptrs);
        char* name = (char*)calloc(1, name_size);
        char* desc = (char*)calloc(1, desc_size);

        ReadProcessMemory(p.h, (LPVOID)(text_ptr_location + string_offset[0].name), name, name_size, NULL);
        if (GetLastError() != 0) {
            printf("Failed to read process memory (code %li)\n", GetLastError());
            return { "", "" };
        }
        ReadProcessMemory(p.h, (LPVOID)(text_ptr_location + string_offset[0].desc), desc, desc_size, NULL);
        if (GetLastError() != 0) {
            printf("Failed to read process memory (code %li)\n", GetLastError());
            return { "", "" };
        }

        skill_text output = { name, desc };
        free(name);
        free(desc);

        return output;
    }
    else {
        return { "", "" };
    }
}

// TODO: This is also some janky bullshit.
bool save_skill_text(pd_meta p, skill_text text, unsigned int id) {
    if (p.h != INVALID_HANDLE_VALUE) {
        SetLastError(0);
        // Memory address of the skill text table & strings
        // static uintptr_t text_section = gstorage_address + (gstorage.unk0 & 0xFFFFF000) + 0x1A000;
        static uintptr_t text_section = p.gstorage_addr + 0x34000;

        // Read section header to check the number of strings
        text_header header = { 0 };
        ReadProcessMemory(p.h, (LPVOID)text_section, &header, sizeof(text_header), NULL);
        if (GetLastError() != 0) {
            printf("Failed to read process memory (code %li)\n", GetLastError());
            return false;
        }

        if (id > header.array_size - 1) {
            return false;
        }

        uintptr_t text_ptr_location = text_section + sizeof(text_header) + ((id - 1) * 0xC);

        text_ptrs string_offset[2] = {0};
        ReadProcessMemory(p.h, (LPVOID)text_ptr_location, &string_offset, sizeof(text_ptrs) * 2, NULL);
        if (GetLastError() != 0) {
            printf("Failed to read process memory (code %li)\n", GetLastError());
            return false;
        }

        uint32_t name_size = text.name.length() + 1;
        uint32_t desc_size = text.desc.length() + 1;
        char* name = text.name.data();
        char* desc = text.desc.data();

        WriteProcessMemory(p.h, (LPVOID)(text_ptr_location + string_offset[0].name), name, name_size, NULL);
        if (GetLastError() != 0) {
            printf("Failed to write process memory (code %li)\n", GetLastError());
            return false;
        }
        WriteProcessMemory(p.h, (LPVOID)(text_ptr_location + string_offset[0].desc), desc, desc_size, NULL);
        if (GetLastError() != 0) {
            printf("Failed to write process memory (code %li)\n", GetLastError());
        }

        return true;
    }
    else {
        return false;
    }
}

bool write_gsdata_to_memory(pd_meta p) {
    if (can_read_memory(p)) {
        // Update version number
        p.gstorage->VersionNum = crc32buf((char*)p.gstorage, sizeof(*p.gstorage));

        WriteProcessMemory(p.h, (LPVOID)p.gstorage_addr, p.gstorage, sizeof(*p.gstorage), NULL);
        DWORD error = GetLastError();
        if (error != 1400 && error != 183 && error != 0) {
            printf("Process Write Error Code: %ld\n", error);
            return false;
        }
        else {
            printf("Wrote skill data to memory.\n");
            return true;
        }
    }
    else { return false; }
}

void install_mod(pd_meta p) {
    if (can_read_memory(p)) {
        for (int n = 0; n < MultiSelectCount; n++) { // Loop through every selected skill pack file
            pack_header1 header = {0};
            FILE* skill_pack = fopen(multiselectpath[n].c_str(), "rb");
            if (skill_pack == nullptr) {
                printf("Failed to open %s\n", multiselectpath[n].c_str());
                continue; // Skip to next mod file
            }
            fread(&header, sizeof(pack_header1), 1, skill_pack);

            atkskill* skills = new atkskill[header.skill_count];
            fread(skills, sizeof(atkskill), header.skill_count, skill_pack);
            for (int i = 0; i < header.skill_count; i++) {
                p.gstorage->skill_array[(skills[i].SkillID - 1)] = skills[i]; // Write skills from pack into gsdata
            }
            pack2_text* text_meta = (pack2_text*) malloc(sizeof(pack2_text) * header.skill_count);
            if (text_meta != nullptr) {
                fread(text_meta, sizeof(pack2_text), header.skill_count, skill_pack);

                // This is really hard to read. Sorry.
                if (header.format_version == 2) {
                    for (uint16_t i = 0; i < header.skill_count; i++) {
                        skill_text original_text = load_skill_text(p, skills[i].SkillTextID + 1);

                        if (original_text.name.length() <= text_meta[i].name_length) {
                            int text_id = skills[i].SkillTextID + 1;
                            char* name = (char*)malloc(text_meta[i].name_length);
                            char* desc = (char*)malloc(text_meta[i].desc_length);
                            fread(name, text_meta[i].name_length, 1, skill_pack);
                            fread(desc, text_meta[i].desc_length, 1, skill_pack);
                            save_skill_text(p, { name, desc }, skills[i].SkillTextID + 1);
                            free(name);
                            free(desc);
                        }
                    }
                }
            }
            // TODO: Test if freeing this breaks anything.
            // free(text_meta);
            delete[] skills;

            fclose(skill_pack);
        }

        write_gsdata_to_memory(p); // Automatically updates version number
    }
}

void toggle_game_pause(pd_meta p) {
    static bool GamePaused = false;
    if (GamePaused) {
        DebugActiveProcessStop(p.pid);
        GamePaused = true;
    }
    else {
        DebugActiveProcess(p.pid);
    }
    GamePaused = !GamePaused;
}
