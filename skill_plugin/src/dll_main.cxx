#include <iostream>
#include <chrono>
#include <thread>
using namespace std::chrono_literals;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <commands.h>
#include "loader.hxx"

void __stdcall plugin_thread(void* plugin_handle) {
    std::ios_base::sync_with_stdio(true);
    command_register(command_load_skill, "skill_loader", "load");

    std::this_thread::sleep_for(3000ms);
    load_skills();
}

__declspec(dllexport) int32_t __cdecl __stdcall DllMain(void* plugin_handle, uint32_t reason, void* reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Disable DLL notifications for new threads starting up, because we have no need to run special code here.
        DisableThreadLibraryCalls((HMODULE)plugin_handle);

        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) plugin_thread, plugin_handle, 0, NULL);
    }
    return TRUE;
}
