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

    init_winapi();
    get_process();
    CreateUI(); // Main UI loop
    CoUninitialize();
    return 0;
}
