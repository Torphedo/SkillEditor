#include "skill_editor.hxx"
#include <iostream>

#include <imgui.h>
#include <imgui_internal.h> // For messing with the viewport for menu bar
#include <imgui/misc/cpp/imgui_stdlib.h> // For std::string input fields
#include <imgui_markdown.h>

#include "winAPI.hxx"
#include "skill_io.h"
#include "text.hxx"

extern "C" {
#include "file.h"
}

static const char* DocumentationAtkBody[50] = {
#include "../res/AttackSkillBody.txt"
};

static const char* DocumentationAtkLabels[50] = {
#include "../res/AttackSkillLabels.txt"
};

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

// ImGui didn't have pre-made functions for short
// or uint8 input boxes, so I made my own.

void InputShort(const char* label, void* p_data, unsigned short step) {
    ImGui::SetNextItemWidth(200);
    ImGui::InputScalar(label, ImGuiDataType_U16, p_data, &step);
}

void InputU32(const char* label, void* p_data, unsigned short step) {
    ImGui::SetNextItemWidth(200);
    ImGui::InputScalar(label, ImGuiDataType_U32, p_data, &step);
}

void InputUInt8(const char* label, void* p_data) {
    static constexpr int step = 1;
    ImGui::SetNextItemWidth(200);
    ImGui::InputScalar(label, ImGuiDataType_U8, p_data, &step);
}

namespace ImGui {
    bool SliderShort(const char* label, uint16_t* v, uint16_t v_min, uint16_t v_max, const char* format, ImGuiSliderFlags flags) {
        return SliderScalar(label, ImGuiDataType_U16, v, &v_min, &v_max, format, flags);
    }
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

    // Load YAML config file
    const char* cfg_path = "labels.yaml";
    char* cfg_data = (char*)file_load(cfg_path);
    if (cfg_data != nullptr) {
        this->custom_labels = user_config(cfg_data);
    }

    // Initialize PD metadata
    get_process(&p);
}

editor::~editor() {
    VirtualFree(p.gstorage, sizeof(*p.gstorage), MEM_RELEASE);
    free(this->cfg_yaml);
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
        flush_to_pd(p);
    }

    if (ImGui::BeginViewportSideBar("MenuBar", viewport, ImGuiDir_Up, height, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Skill Pack", "Ctrl-N")) {
                    if (SUCCEEDED(file_multiple_select_dialog())) {
                        NewSkillPack = true;
                    }
                    else {
                        printf("Skill selection canceled.\n");
                    }
                }
                if (ImGui::BeginMenu("Open")) {
                    if (ImGui::MenuItem("Skill (From Memory)", "Ctrl-L")) {
                        if (game_available) {
                            IDSelection = true;
                        }
                    }
                    if (ImGui::MenuItem("Skill File")) {
                        ID = load_attack_skill(p, ID);
                        AttackSkillEditor = true; // Open the Attack Skill Editor window
                    }
                    if (ImGui::MenuItem("Install Skill Pack")) {
                        if (game_available) {
                            if (SUCCEEDED(file_multiple_select_dialog())) { // Open a multiple file open dialog
                                install_mod(p);
                                for (int i = 0; i < MultiSelectCount; i++) {
                                    std::cout << "Installed skill pack " << multiselectpath[i] << ".\n";
                                }
                            }
                            else {
                                printf("File selection canceled.\n");
                            }
                        }
                        else {
                            printf("Can't access game's skill data in memory, cancelling skill pack install.\n");
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Save To File", "Shift + S")) {
                    text_prompt = true;
                }
                if (ImGui::MenuItem("Save As", "Ctrl + S")) {
                    skill_select();
                    text_prompt = true;
                }

                if (ImGui::MenuItem("Exit", "Alt + F4")) {
                    return 1;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window")) {
                ImGui::MenuItem("Attack Skill Editor", nullptr, &AttackSkillEditor);
                if (ImGui::MenuItem("Skill Hex Editor")) {
                    if (game_available) {
                        HexEditor = !HexEditor;
                    }
                }
                ImGui::MenuItem("Documentation", nullptr, &Documentation);
                if (ImGui::MenuItem("Text Edit", nullptr, &text_edit)) {
                    update_process(&p, false); // Refresh skill data address & game handle

                    skill_text text = load_skill_text(p, ID);
                    current_name = text.name;
                    current_desc = text.desc;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options")) {
                if (ImGui::MenuItem("Freeze/Unfreeze Phantom Dust", "F4")) {
                    toggle_game_pause(p);
                }
                ImGui::Checkbox("Input Box Limits", &limitless);
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

    // ImGui::ShowDemoWindow();

    const bool control = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    const bool shift = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);

    if (control && ImGui::IsKeyPressed(ImGuiKey_L)) {
        if (game_available) {
            IDSelection = true;
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F4)) {
        toggle_game_pause(p);
    }
    // Save
    if (ImGui::IsKeyPressed(ImGuiKey_S)) {
        // Save As
        if (control) {
            if (skill_select()) {
                text_prompt = true;
            }
        }
        // Save
        else if (shift) {
            text_prompt = true;
        }
    }
    if (control && ImGui::IsKeyPressed(ImGuiKey_N, false) && !NewSkillPack) {
        if (SUCCEEDED(file_multiple_select_dialog())) {
            NewSkillPack = true;
        }
    }

    if (text_prompt) {
        // Stupid and convoluted. If we don't use OpenPopup(), it will create it with the default
        // frame ("Debug" label). But it breaks if called from a menu item, so we need this.
        ImGui::OpenPopup("save_text_prompt");
    }

    if (ImGui::BeginPopup("save_text_prompt")) {
        ImGui::Text("Save text to the skill file?");
        if (ImGui::Button("No")) {
            save_skill_to_file(p, ID, false);
            text_prompt = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Yes")) {
            save_skill_to_file(p, ID, true);
            text_prompt = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (IDSelection) {
        // Temporary storage to hold an ID before actually updating the selected ID
        static s32 temp_id = 0;
        static bool hex = false;

        ImGui::Begin("Input a skill ID: ");
        ImGui::Checkbox("Hexidecimal", &hex);
        ImGui::InputInt("ID", &temp_id, 1, 1, ImGuiInputTextFlags_CharsHexadecimal * hex);
        if (ImGui::Button("Open")) {
            ID = temp_id;
            printf("Loaded skill with ID %d\n", ID);
            IDSelection = false;      // Close this window
            AttackSkillEditor = true; // Opens the Attack Skill Editor window
        }
        ImGui::End();
    }

    if (HexEditor) {
        hex_edit.OptShowAscii = false;
        hex_edit.DrawWindow("Hex Editor", &p.gstorage->skill_array[ID - 1], 144);
    }

    if (AttackSkillEditor) {
        // Render the editor w/ user-controlled labels
        atkskill* skill = &p.gstorage->skill_array[ID - 1];
        if (ImGui::Begin("Skill Editor", &this->AttackSkillEditor)) {
            this->custom_labels.render_editor(skill);
        }
        ImGui::End();
    }

    if (Documentation) {
        ImGui::SetNextWindowSize(ImVec2(850, 650), ImGuiCond_FirstUseEver);
        ImGui::Begin("Documentation", &Documentation);

        if (ImGui::BeginTabBar("DocTabs")) {
            if (ImGui::BeginTabItem("Attack Skills")) {
                static uint16_t select_idx = 0;
                ImGui::BeginChild("left pane", ImVec2(200, 0), true);
                for (unsigned short i = 0; i < (unsigned short)IM_ARRAYSIZE(DocumentationAtkLabels); i++) {
                    // Selectable object for every string in the array
                    if (ImGui::Selectable(DocumentationAtkLabels[i])) {
                        select_idx = i;
                    }
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("Doc Text", ImVec2(600, 0), false);
                // Renders the item name as an H1, then the description on a new line.
                std::string md = "# ";
                md += DocumentationAtkLabels[select_idx];
                md += DocumentationAtkBody[select_idx];
                ImGui::Markdown(md.c_str(), md.length(), mdConfig);

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Skill Editor")) {
                static uint16_t select_idx = 0;
                ImGui::BeginChild("left pane", ImVec2(200, 0), true);
                for (unsigned short i = 0; i < (unsigned short)IM_ARRAYSIZE(DocumentationProgramLabels); i++) {
                    // Selectable object for every string in the array
                    if (ImGui::Selectable(DocumentationProgramLabels[i])) {
                        select_idx = i;
                    }
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("Doc Text", ImVec2(600, 0), false);
                // Renders the item name as an H1, then the description on a new line.
                std::string md = "# ";
                md += DocumentationProgramLabels[select_idx];
                md += DocumentationProgramBody[select_idx];
                ImGui::Markdown(md.c_str(), md.length(), mdConfig);

                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }


    if (NewSkillPack) {
        static char packname_internal[32] = {0}; // Mod name that will be stored in the binary
        ImGui::Begin("Enter a name for your skill pack: ");
        ImGui::InputText("Skill Pack Name", packname_internal, 32);
        if (ImGui::Button("Save")) {
            save_skill_pack(packname_internal);
            NewSkillPack = false;
        }
        ImGui::End();
    }

    if (text_edit) {
        ImGui::Begin("Skill Text Editor", &text_edit);

        // The input field modifies a std::string, so there's no length limit.
        ImGui::InputText("Skill Name", &current_name);
        ImGui::InputText("Skill Description", &current_desc);

        static uint16_t text_id = 0;
        uint16_t cache = text_id; // Previously selected skill ID
        text_id = p.gstorage->skill_array[ID - 1].SkillTextID + 1;

        if (ImGui::Button("Reload") || cache != text_id) {
            skill_text text = load_skill_text(p, text_id);
            current_name = text.name;
            current_desc = text.desc;
        }

        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            skill_text text = {current_name, current_desc};
            save_skill_text(p, text, text_id);
        }
        ImGui::End();
    }
    return 0;
}