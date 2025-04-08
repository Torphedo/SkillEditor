#include <imgui.h>
#include <imgui_internal.h> // For messing with the viewport for menu bar
#include <imgui/misc/cpp/imgui_stdlib.h> // For std::string input fields
#include <imgui_markdown.h>

#include "skill_editor.hxx"
#include "winAPI.hxx"
#include "mods.hxx"
#include "text.hxx"

#include <common/file.h>

// I'd normally keep this near the top, but c4 and windows.h are mortal enemies.
// c4 always has to be included first, and it comes in via skill_editor.hxx.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

static const char* DocumentationProgramBody[] = {
#include "../res/SkillEditorBody.txt"
};

static const char* DocumentationProgramLabels[] = {
#include "../res/SkillEditorLabels.txt"
};

void Tooltip(const char* text) {
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", text);
    }
    ImGui::TableNextColumn();
}

// Markdown setup stuff
void LinkCallback(ImGui::MarkdownLinkCallbackData data_) {
    std::string url(data_.link, data_.linkLength);
    if (!data_.isImage) {
        ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

editor::editor() {
    mdConfig.linkCallback = LinkCallback;
    mdConfig.tooltipCallback = nullptr;
    mdConfig.linkIcon = nullptr; // Was "ICON_FA_LINK" in example
    mdConfig.headingFormats[0] = { nullptr, true };
    mdConfig.headingFormats[1] = { nullptr, true };
    mdConfig.headingFormats[2] = { nullptr, false };
    mdConfig.userData = nullptr;

    // Initialize PD metadata
    get_process(&p);
}

editor::~editor() {
    VirtualFree(p.gstorage, sizeof(*p.gstorage), MEM_RELEASE);
}

skill_t* editor::cur_skill() {
    return &p.gstorage->skill_array[ID - 1];
}

int editor::draw() {
    if (ID == 0) {
        ID++;
    }
    ImGuiViewportP* viewport = (ImGuiViewportP*)ImGui::GetMainViewport();
    float height = ImGui::GetFrameHeight();

    ImGui::DockSpaceOverViewport(); // Enable docking
    const bool game_available = handle_still_valid(p.h);
    const bool game_running = is_running();

    if (!game_available) {
        update_process(&p, false);
    }

    if (game_running && game_available) {
        const bool gstorage_changed = flush_to_pd(p);

    }

    const bool control = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    const bool shift = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);

    bool load_from_disk = control && shift && ImGui::IsKeyPressed(ImGuiKey_L, false);

    if (ImGui::BeginViewportSideBar("MenuBar", viewport, ImGuiDir_Up, height, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {

                if (ImGui::BeginMenu("Open")) {
                    load_from_disk |= ImGui::MenuItem("Skill (From file)", "Ctrl-Shift-L");
                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Exit", "Alt + F4")) {
                    return 1;
                }
                ImGui::EndMenu();
            }
            if (!handle_still_valid(p.h)) {
                ImGui::SameLine(viewport->Size.x - 300);
                ImGui::TextColored({ 255, 0, 0, 255 }, "No Phantom Dust instance detected!");
            }
            else {
                ImGui::SameLine(viewport->Size.x - 15 - 575);
                if (p.h == INVALID_HANDLE_VALUE || p.h == NULL) {
                    ImGui::TextColored({ 255, 0, 0, 255 }, "No handle to process!");
                }
                else if (!game_available) {
                    ImGui::TextColored({ 255, 0, 0, 255 }, "Can't read from process!");
                }
                ImGui::SameLine(viewport->Size.x - 425);

                if (ImGui::Button("Refresh process")) {
                    printf("Refreshing process info & skill/text data... \n");
                    update_process(&p, true);
                }

                ImGui::Text("Phantom Dust Process ID:");
                ImGui::TextColored({ 0, 255, 0, 255 }, "%u", p.pid);
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
    }

    if (load_from_disk) {
        if (!game_available) {
            printf("Can't access game's skill data in memory, cancelling skill pack install.\n");
        } else {
            // Open a multiple file open dialog
            if (!SUCCEEDED(file_multiple_select_dialog())) {
                printf("File selection canceled.\n");
            } else {
                install_mod(p);
            }
        }
    }

    return 0;
}