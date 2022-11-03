#include <filesystem>
#include <fstream>

#include "imgui/imgui_backend.h"
#include "winAPI.h"
#include "memory-editing.h"

static const char* ImGuiConfig = {
#include "../res/imgui-config.txt"
};

int main()
{
    // Write imgui config if it doesn't already exist
    if (!std::filesystem::exists("imgui.ini"))
    {
        std::ofstream config;
        config.open("imgui.ini");
        config << ImGuiConfig;
        config.close();
    }

    // Initialize something for WinAPI file explorer calls
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    CreateUI(); // Main UI loop
    CoUninitialize();
    return 0;
}
