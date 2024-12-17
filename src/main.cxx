#include <cstdio>
#include <filesystem>

#include "userlabels.hxx"
#include "gui_loop.hxx"
#include "winAPI.hxx"

extern "C" {
#include "file.h"
}

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

    printf("Starting GUI.\n\n");

    gui_main(); // Main UI loop
    CoUninitialize();
    return 0;
}
