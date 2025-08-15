#include <cstdio>
#include <filesystem>

#include "gui_loop.hxx"
#include "mods.hxx"
#include "common/logging.h"

static const char ImGuiConfig[] = {
#include "../res/imgui-config.txt"
};

int main(int argc, char** argv) {
    // Write imgui config if it doesn't already exist
    if (!std::filesystem::exists("imgui.ini")) {
        FILE* config_out = fopen("imgui.ini", "wb");
        if (config_out != nullptr) {
            fwrite(ImGuiConfig, sizeof(ImGuiConfig), 1, config_out);
            fclose(config_out);
        }
    }

    if (argc > 1) {
        if (strcmp(argv[1], "--install") == 0) {
            pd_meta p = {};
            if (!get_process(&p)) {
                LOG_MSG(error, "Failed to connect to Phantom Dust\n");
                return EXIT_FAILURE;
            }
            update_process(&p, true);
            flush_to_pd(p, true);

            for (u32 i = 2; i < argc; i++) {
                const char* modpath = argv[i];
                std::string modpathStr = modpath;
                install_mod(p, &modpathStr, 1);
            }

            flush_to_pd(p, true);
            return  EXIT_SUCCESS;
        }
    }

    printf("Looking for Phantom Dust...\n");

    printf("Starting GUI.\n\n");

    gui_main(); // Main UI loop
    return 0;
}
