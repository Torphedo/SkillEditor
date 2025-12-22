#include <iostream>
#include <chrono>
#include <thread>
using namespace std::chrono_literals;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MinHook.h>

#include <common/logging.h>
#include <common/int.h>
#include <commands.h>
#include "loader.hxx"

typedef void (*LOAD_SKILLS)(void* unknown_ptr, int* unknown_int_ptr, int unknown_int);
LOAD_SKILLS address_load_skills = nullptr;
LOAD_SKILLS original_load_skills = nullptr;

// Address of the function that loads skills
const uintptr_t offset_load_skills = 0x17E0A0;

void hook_load_skills(void* unknown_ptr, int* unknown_int_ptr, int unknown_int) {
    (original_load_skills)(unknown_ptr, unknown_int_ptr, unknown_int);
    load_skills();
}

void __stdcall plugin_thread(void* plugin_handle) {
    const uintptr_t pduwp = (uintptr_t)GetModuleHandle("PDUWP.exe");
    address_load_skills = (LOAD_SKILLS) (pduwp + offset_load_skills);

    const int res = MH_Initialize();
    if (res != MH_OK && res != MH_ERROR_ALREADY_INITIALIZED) {
        LOG_MSG(error, "Failed to start MinHook.\n");
    }

    if (MH_CreateHook(address_load_skills, &hook_load_skills, (void**) &original_load_skills) != MH_OK) {
        LOG_MSG(error, "Failed to create hook.\n");
    }
    if (MH_EnableHook(address_load_skills) != MH_OK) {
        LOG_MSG(error, "Failed to enable hook.\n");
    }

    std::ios_base::sync_with_stdio(true);
    command_register(command_load_skill, "skill_loader", "load");
}

__declspec(dllexport) int32_t __cdecl __stdcall DllMain(void* plugin_handle, uint32_t reason, void* reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Disable DLL notifications for new threads starting up, because we have no need to run special code here.
        DisableThreadLibraryCalls((HMODULE)plugin_handle);

        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) plugin_thread, plugin_handle, 0, NULL);
    }
    return TRUE;
}
