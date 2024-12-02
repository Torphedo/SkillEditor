#include <cstdio>

#include <filesystem>

#include "imgui/imgui_backend.h"
#include "winAPI.h"
#include "memory-editing.h"

static const char ImGuiConfig[] = {
#include "../res/imgui-config.txt"
};

int main() {
    // Write imgui config if it doesn't already exist
    if (!std::filesystem::exists("imgui.ini")) {
        FILE* config_out = fopen("imgui.ini", "wb");
        if (config_out != nullptr) {
            fwrite(ImGuiConfig, sizeof(ImGuiConfig), 1, config_out);
            fclose(config_out);
        }
    }


    init_winapi();
    printf("Looking for Phantom Dust...\n");
    pd_meta p = {0};
    get_process(&p);

    printf("Starting GUI.\n\n");

    CreateUI(p); // Main UI loop
    CoUninitialize();
    VirtualFree(p.gstorage, sizeof(*p.gstorage), MEM_RELEASE);
    return 0;
}
