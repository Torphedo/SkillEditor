#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <memoryapi.h>

#include <common/crc32.h>
#include <common/logging.h>
#include <common/path.h>

#include "remote_pd.h"
#include "structs.h"

void win32_print_error_msg(DWORD err_code) {
    const char* msg = NULL;
    DWORD format_result = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_MAX_WIDTH_MASK, 0, err_code, 0, (LPTSTR)&msg, 1, NULL);

    printf("\"%s\"", (msg == NULL) ? "[message missing]" : msg);

    if (format_result != 0) {
        LocalFree((void*)msg);
    }
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

bool set_debug_privilege(bool state) {
    HANDLE hToken = NULL;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
        return false;
    }
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        return false;
    }

    TOKEN_PRIVILEGES tokenPriv = {
        .PrivilegeCount = 1,
        .Privileges[0] = {
            .Luid = luid,
            .Attributes = state ? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_REMOVED
        }
    };
    if (!AdjustTokenPrivileges(hToken, false, &tokenPriv, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        return false;
    }
    return true;
}

uintptr_t remote_module_base_addr(HANDLE h) {
    HMODULE modules[1024] = {0};
    DWORD bytes_needed = 0;

    // Get path to base module
    char base_exe_name[MAX_PATH] = {0};
    HMODULE base_exe_module = 0;
    DWORD base_exe_len = ARRAY_SIZE(base_exe_name);
    bool result = QueryFullProcessImageNameA(h, 0, base_exe_name, &base_exe_len);
    path_get_filename(base_exe_name, base_exe_name);

    if (!result) {
        const DWORD err_code = GetLastError();
        LOG_MSG(error, "Failed to get executable name of the other process (error code %d, Windows says: ", err_code);
        win32_print_error_msg(err_code);
        printf(")\n");
    }

    bool found_module = false;
    if (EnumProcessModules(h, modules, sizeof(modules), &bytes_needed)) {
        for (uint32_t i = 0; i < (bytes_needed / sizeof(HMODULE)); i++) {
            char module_name[MAX_PATH] = {0};

            result = GetModuleBaseName(h, modules[i], module_name, ARRAY_SIZE(module_name));
            if (!result) {
                LOG_MSG(error, "Failed to get filename for a module in the other process (error code %d)\n", GetLastError());
                continue;
            }

            if (strnicmp(module_name, base_exe_name, MAX_PATH) == 0) {
                // The remote module handle is just the pointer to the
                // module in the other process' address space
                base_exe_module = modules[i];
                found_module = true;
                break;
            }
        }
    }

    if (!found_module) {
        LOG_MSG(error, "Couldn't find base address of main module '%s' [process handle %p]\n", base_exe_name, h);
    }

    return (uintptr_t)base_exe_module;
}

bool is_running() {
    return get_pid_by_name("PDUWP.exe") != 0;
}

remote_region alloc_remote_region(u32 size, uintptr_t remote_addr, const char* name, HANDLE h) {
    remote_region out = {
        .name = name,
        .size = size,
        .remote_addr = remote_addr,
    };
    if (size > REMOTE_REGION_MAX_SIZE) {
        LOG_MSG(error, "Remote region is 0x%X bytes, we can only handle up to 0x%X!\n", size, REMOTE_REGION_MAX_SIZE);
        return out;
    }

    const DWORD alloc_flags = MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH;
    out.local_data = VirtualAlloc(NULL, size, alloc_flags, PAGE_READWRITE);
    if (!out.local_data) {
        LOG_MSG(error, "Failed to allocate local copy of %x byte memory region '%s'!\n", size, name);
        return out;
    }

    SIZE_T actual_read = 0;
    bool result = ReadProcessMemory(h, (LPVOID)remote_addr, out.local_data, size, &actual_read);
    ResetWriteWatch(out.local_data, size);

    if (!result) {
        const DWORD err_code = GetLastError();
        LOG_MSG(error, "Failed to read '%s' from Phantom Dust (error code %ld, remote pointer %p)\n", name, err_code, (void*)remote_addr);
        LOG_MSG(info, "Windows says ");
        win32_print_error_msg(err_code);
        printf("\n");

        bool valid = handle_still_valid(h);
        const char* valid_msg = valid ? "still valid" : "no longer valid";
        const float ratio = ((float)actual_read / size) * 100;
        LOG_MSG(debug, "Debug info: Request is for handle 0x%x (which is %s). We wanted 0x%x bytes, and got 0x%x bytes (%.0f%%).\n", h, valid_msg, size, actual_read, ratio);

        if (err_code != 0) {
            SetLastError(0);
        }

        return out;
    }

    return out;
}

bool flush_remote_region(const remote_region* reg, HANDLE h) {
    if (!reg->local_data) {
        LOG_MSG(warning, "No data to flush for remote region '%s'\n", reg->name);
        return false;
    }

    void* dirty_pages[REMOTE_REGION_MAX_PAGES] = {0};
    ULONG_PTR address_count = ARRAYSIZE(dirty_pages);
    DWORD page_size = 0;
    GetWriteWatch(WRITE_WATCH_FLAG_RESET, reg->local_data, reg->size, dirty_pages, &address_count, &page_size);

    for (int i = 0; i < address_count; i++) {
        const ptrdiff_t offset = (u8*)dirty_pages[i] - (u8*)reg->local_data;
        WriteProcessMemory(h, (void*)(reg->remote_addr + offset), dirty_pages[i], page_size, NULL);
    }

    const bool need_write = address_count > 0;
    return need_write;
}

void free_remote_region(remote_region* reg) {
    VirtualFree(reg->local_data, reg->size, MEM_RELEASE);
    memset(reg, 0, sizeof(*reg));
}

bool get_process(pd_meta* p) {
    set_debug_privilege(true);
    p->pid = get_pid_by_name("PDUWP.exe");
    if (p->pid == 0) {
        // The game isn't running, any handles we had are now invalid.
        p->h = INVALID_HANDLE_VALUE;
    }

    // Open game process
    DWORD access = PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | SYNCHRONIZE;
    p->h = OpenProcess(access, FALSE, p->pid);
    // We still want the UI to run when the game is closed/crashed, so we'll
    // still allocate the buffers on our side. So we won't bother checking for
    // failure here, and let the parts that require access to the game process
    // safely fail.

    LOG_MSG(debug, "PDUWP handle 0x%p [PID %d]\n", p->h, p->pid);

    const uintptr_t base_exe_module = remote_module_base_addr(p->h);
    const uintptr_t gstorage_addr = ((uintptr_t)base_exe_module + gstorage_offset);
    const uintptr_t anim_addr = ((uintptr_t)base_exe_module + anim_profiles_offset);

    if (!p->gstorage.local_data) {
        p->gstorage = alloc_remote_region(GSDATA_SIZE, gstorage_addr, "Skill Data", p->h);
    }

    if (!p->anim_profiles.local_data) {
        const u32 size = sizeof(anim_profile) * ANIMATION_PROFILE_COUNT;
        p->anim_profiles = alloc_remote_region(size, anim_addr, "Animation profiles", p->h);
    }

    return p;
}

bool flush_to_pd(pd_meta p, bool use_vanilla_version) {
    gsdata* gstorage = p.gstorage.local_data;

    // Trigger a flush whenever the version number needs to change
    if (use_vanilla_version) {
        if (gstorage->VersionNum != PD_VERSION_NUMBER) {
            gstorage->VersionNum = PD_VERSION_NUMBER;
        }
    } else if (gstorage->VersionNum == PD_VERSION_NUMBER) {
        gstorage->VersionNum = 0;
    }
    const bool need_gsdata_write = flush_remote_region(&p.gstorage, p.h);
    const bool need_anim_write = flush_remote_region(&p.anim_profiles, p.h);

    // If there's at least 1 page that changed, we need to copy some data
    if (need_gsdata_write && !use_vanilla_version) {
        // Make sure the current version number doesn't affect the hash
        gstorage->VersionNum = 0;

        // Update version number
        gstorage->VersionNum = crc32buf((u8*)gstorage, sizeof(*gstorage));

        // Copy the change over
        flush_remote_region(&p.gstorage, p.h);
    }

    return need_gsdata_write || need_anim_write;
}

bool handle_still_valid(HANDLE h) {
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }

    // If waiting on the process for 0ms times out, process is still running.
    // If it returns something else, the process was terminated.
    DWORD result = WaitForSingleObject(h, 0);
    /*
    if (result == WAIT_FAILED) {
        DWORD err = GetLastError();
        printf("Process check failed with code %ld\n", err);
        SetLastError(0);
    }
    */
    return (result == WAIT_TIMEOUT);
}

void update_process(pd_meta* p, bool force) {
    // Don't bother updating if the game is still running
    // (meaning our handle & remote gsdata pointer are still good)
    // Caller can force an update (to refresh gsdata, for example)
    if (!force && handle_still_valid(p->h)) {
        return;
    }

    if (p->h != INVALID_HANDLE_VALUE && p->h != NULL) {
        // Clean up our old handle before we open a new one
        CloseHandle(p->h);
    }
    free_remote_region(&p->gstorage);
    free_remote_region(&p->anim_profiles);

    // Update everything
    get_process(p);
}

bool can_read_memory(pd_meta p) {
    if (!handle_still_valid(p.h)) {
        return false;
    }

    // Try to read memory
    unsigned int buf = 0;
    ReadProcessMemory(p.h, (LPVOID)p.gstorage.remote_addr, &buf, 1, NULL);
    const DWORD error = GetLastError();
    SetLastError(0);
    return (error == 298) || (error == 0);
}

void toggle_game_pause(pd_meta p) {
    static bool GamePaused = false;
    if (GamePaused) {
        DebugActiveProcessStop(p.pid);
    } else {
        DebugActiveProcess(p.pid);
    }
    GamePaused = !GamePaused;
}
