#include <Windows.h>
#include <shobjidl.h> 
#include <tlhelp32.h>
#include <psapi.h>

#include "memory-editing.h"
#include "structs.h"
#include "winAPI.h"

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
    if (!still_running(p.h)) {
        return false;
    }
    unsigned char buf = 0;
    ReadProcessMemory(p.h, (LPVOID)p.gstorage_addr, &buf, 1, NULL);
    DWORD error = GetLastError();
    SetLastError(0);
    return (error == 298) || (error == 0);
}

pd_meta get_process() {
    pd_meta out = {0};
    if (!is_running()) {
        return out;
    }
    out.pid = get_pid_by_name("PDUWP.exe");

    // Open game process
    DWORD access = PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | SYNCHRONIZE;
    out.h = OpenProcess(access, FALSE, out.pid);
    if (out.h == INVALID_HANDLE_VALUE) {
        return out;
    }
    printf("PDUWP handle 0x%p\n", out.h);

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
        SetLastError(0);
    }

    return out;
}

bool still_running(void* handle) {
    // If waiting on the process for 0ms times out, process is still running.
    // If it returns something else, the process was terminated.
    DWORD result = WaitForSingleObject(handle, 0);
    if (result == WAIT_FAILED) {
        /*
            DWORD err = GetLastError();
            printf("Process check failed with code %ld\n", err);
        */
        SetLastError(0);
    }
    return (result == WAIT_TIMEOUT);
}

void update_process(pd_meta* p, bool force) {
    // Don't bother updating if the game is still running
    // (meaning our handle & remote gsdata pointer are still good)
    // Caller can force an update (to refresh gsdata, for example)
    if (!force && still_running(p->h)) {
        return;
    }

    // Update everything
    if (p->h != INVALID_HANDLE_VALUE && p->h != NULL) {
        CloseHandle(p->h);
    }
    // TODO: This causes use-after-free crash when we close the game with a
    // text edit window open. Make sure game is opened again before deleting
    free(p->gstorage);
    *p = get_process();
}

