#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <memoryapi.h>

#include <common/crc32.h>
#include <common/logging.h>

#include "remote_pd.hxx"
#include "structs.h"
#include "winAPI.hxx"

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

uintptr_t remote_module_base_addr(HANDLE h) {
    HMODULE modules[1024] = {0};
    DWORD bytes_needed = 0;

    // Get path to base module
    char base_exe_name[MAX_PATH] = {0};
    HMODULE base_exe_module = 0;
    GetModuleFileNameEx(h, NULL, base_exe_name, sizeof(base_exe_name) / sizeof(TCHAR));

    if (EnumProcessModules(h, modules, sizeof(modules), &bytes_needed)) {
        for (uint32_t i = 0; i < (bytes_needed / sizeof(HMODULE)); i++) {
            char module_name[MAX_PATH] = {0};

            if (GetModuleFileNameExA(h, modules[i], module_name, sizeof(module_name) / sizeof(TCHAR))) {
                // Check name against the base EXE name
                if (strncmp(module_name, base_exe_name, MAX_PATH) == EXIT_SUCCESS) {
                    base_exe_module = modules[i];
                    break;
                }
            }
        }
    }

    if (base_exe_module == INVALID_HANDLE_VALUE) {
        printf("Couldn't find PDUWP.exe base address.\n");
        return 0;
    }

    return (uintptr_t)base_exe_module;
}

bool is_running() {
    return get_pid_by_name("PDUWP.exe") != 0;
}

void win32_print_error_msg(DWORD err_code) {
    const char* msg = nullptr;
    DWORD format_result = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_MAX_WIDTH_MASK, 0, err_code, 0, (LPTSTR)&msg, 1, nullptr);

    printf("\"%s\"", (msg == nullptr) ? "[message missing]" : msg);

    if (format_result != 0) {
        LocalFree((void*)msg);
    }
}

bool get_process(pd_meta* p) {
    if (p->gstorage == nullptr) {
        // There's no existing gsdata buffer, we need to allocate it
        DWORD alloc_flags = MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH;
        p->gstorage = (gsdata*)VirtualAlloc(nullptr, sizeof(*p->gstorage), alloc_flags, PAGE_READWRITE);
    }

    p->pid = get_pid_by_name("PDUWP.exe");
    if (p->pid == 0) {
        // The game isn't running, any handles we had are now invalid.
        p->h = INVALID_HANDLE_VALUE;
        return false;
    }

    // Open game process
    DWORD access = PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | SYNCHRONIZE;
    p->h = OpenProcess(access, FALSE, p->pid);
    if (p->h == INVALID_HANDLE_VALUE) {
        return false;
    }
    printf("PDUWP handle 0x%p\n", p->h);

    const uintptr_t base_exe_module = remote_module_base_addr(p->h);
    p->gstorage_addr = ((uintptr_t)base_exe_module + gstorage_offset);

    const u32 requested_size = sizeof(*p->gstorage);
    SIZE_T actual_read = 0;
    bool result = ReadProcessMemory(p->h, (LPVOID)p->gstorage_addr, p->gstorage, requested_size, &actual_read);
    ResetWriteWatch((void*)p->gstorage, requested_size);
    if (!result) {
        const DWORD err_code = GetLastError();
        LOG_MSG(error, "Failed to read data from Phantom Dust (error code %ld)\n", err_code);
        LOG_MSG(info, "Windows says ");
        win32_print_error_msg(err_code);
        printf("\n");

        bool valid = handle_still_valid(p->h);
        const char* valid_msg = valid ? "still valid" : "no longer valid";
        const float ratio = ((float)actual_read / requested_size) * 100;
        LOG_MSG(debug, "Debug info: Request is for handle 0x%x (which is %s). We wanted 0x%x bytes, and got 0x%x bytes (%.0f%%).\n", p->h, valid_msg, requested_size, actual_read, ratio);

        if (err_code != 0) {
            SetLastError(0);
        }

        return false;
    }

    return p;
}

bool flush_to_pd(pd_meta p) {
    if (p.gstorage == nullptr) {
        printf("No data to write...\n");
        return false;
    }

    // The data we work with is ~274KiB, and getting pointers to 100 dirty pages
    // allows for 400KiB of watched data to change without missing anything. We
    // can probbaly write a loop to sync an unlimited amount of data between
    // processes, but this is fine for our use case.
    void* dirty_pages[100] = {};
    ULONG_PTR address_count = ARRAYSIZE(dirty_pages);
    DWORD page_size = 0;
    GetWriteWatch(WRITE_WATCH_FLAG_RESET, p.gstorage, sizeof(*p.gstorage), dirty_pages, &address_count, &page_size);

    // If there's at least 1 page that changed, we need to copy some data
    const bool need_write = address_count > 0;
    if (need_write) {
        // Make sure the current version number doesn't affect the hash
        p.gstorage->VersionNum = 0;

        // Update version number
        p.gstorage->VersionNum = crc32buf((u8*)p.gstorage, sizeof(*p.gstorage));

        // We have to update the first page manually here
        WriteProcessMemory(p.h, (void*)(p.gstorage_addr), p.gstorage, page_size, nullptr);

        // Don't trigger the write watch again from editing version
        ResetWriteWatch(p.gstorage, sizeof(*p.gstorage));
    }

    for (int i = 0; i < address_count; i++) {
        const ptrdiff_t offset = (char*)dirty_pages[i] - (char*)p.gstorage;
        WriteProcessMemory(p.h, (void*)(p.gstorage_addr + offset), dirty_pages[i], page_size, NULL);
    }

    return need_write;
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

    // Update everything
    get_process(p);
}

bool can_read_memory(pd_meta p) {
    if (!handle_still_valid(p.h)) {
        return false;
    }

    // Try to read memory
    unsigned int buf = 0;
    ReadProcessMemory(p.h, (LPVOID)p.gstorage_addr, &buf, 1, nullptr);
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