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
        static u16 skill_id_cache = this->ID;
        const bool skill_id_changed = skill_id_cache != this->ID;
        const bool gstorage_changed = flush_to_pd(p);

        if (skill_id_changed) {
            // Reset saved ID
            skill_id_cache = this->ID;
        }

        if (gstorage_changed || skill_id_changed) {
            // The skill labels might be out of date. This is a little
            // trigger-happy, but it's not super expensive to update all our
            // labels. Better to be a little slow and always correct than super
            // fast and out of sync.
            this->custom_labels.update_unconditional_labels();
            this->custom_labels.update_conditional_labels(*(this->cur_skill()));
        }
    }

    const bool control = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    const bool shift = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);

    bool new_skill_pack = control && ImGui::IsKeyPressed(ImGuiKey_N, false);
    bool load_from_mem = control && ImGui::IsKeyPressed(ImGuiKey_L, false);
    bool save_as = control && shift && ImGui::IsKeyPressed(ImGuiKey_S, false);
    bool save_normal = control && ImGui::IsKeyPressed(ImGuiKey_S, false);
    bool toggle_freeze_game = ImGui::IsKeyPressed(ImGuiKey_F4, false);

    if (ImGui::BeginViewportSideBar("MenuBar", viewport, ImGuiDir_Up, height, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                new_skill_pack |= ImGui::MenuItem("New Skill Pack", "Ctrl-N");

                if (ImGui::BeginMenu("Open")) {
                    load_from_mem |= ImGui::MenuItem("Skill (From Memory)", "Ctrl-L");

                    if (ImGui::MenuItem("Skill File or Pack")) {
                        if (!game_available) {
                            printf("Can't access game's skill data in memory, cancelling skill pack install.\n");
                        } else {
                            // Open a multiple file open dialog
                            if (!SUCCEEDED(file_multiple_select_dialog())) {
                                printf("File selection canceled.\n");
                            } else {
                                install_mod(p);
                                AttackSkillEditor = true; // Open the Attack Skill Editor window
                            }
                        }
                    }
                    if (ImGui::MenuItem("Install Skill Pack")) {
                    }
                    ImGui::EndMenu();
                }
                save_normal |= ImGui::MenuItem("Save To File", "Shift + S");
                save_as |= ImGui::MenuItem("Save As", "Ctrl + S");

                if (ImGui::MenuItem("Exit", "Alt + F4")) {
                    return 1;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window")) {
                ImGui::MenuItem("Attack Skill Editor", nullptr, &AttackSkillEditor);
                ImGui::MenuItem("Skill Hex Editor", nullptr, &HexEditor);
                ImGui::MenuItem("Documentation", nullptr, &Documentation);
                if (ImGui::MenuItem("Text Edit", nullptr, &text_edit)) {
                    update_process(&p, false); // Refresh skill data address & game handle

                    const skill_text text = get_skill_text(p, ID);
                    current_name = text.name;
                    current_desc = text.desc;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options")) {
                toggle_freeze_game |= ImGui::MenuItem("Freeze/Unfreeze Phantom Dust", "F4");
                ImGui::Checkbox("Remove Input Box Limits", &limitless);
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
    if (new_skill_pack) {
        if (SUCCEEDED(file_multiple_select_dialog())) {
            save_skill_pack();
        } else {
            printf("Skill selection canceled.\n");
        }
    }

    if (load_from_mem) {
        if (game_available) {
            IDSelection = true;
        }
    }

    if (toggle_freeze_game) {
        toggle_game_pause(p);
    }

    // Saving
    if (save_normal || save_as) {
        if (save_as) {
            skill_select();
        }
        text_prompt = true;
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

        ImGui::Begin("Input a skill ID: ", &IDSelection);
        ImGui::Checkbox("Hexadecimal", &hexadecimal_ids);
        ImGui::InputInt("ID", &temp_id, 1, 1, ImGuiInputTextFlags_CharsHexadecimal * hexadecimal_ids);
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
        if (ImGui::Begin("Skill Editor", &this->AttackSkillEditor)) {
            this->custom_labels.render_editor(this->cur_skill(), limitless);
        }
        ImGui::End();
    }

    if (Documentation) {
        ImGui::SetNextWindowSize(ImVec2(850, 650), ImGuiCond_FirstUseEver);
        ImGui::Begin("Documentation", &Documentation);

        if (ImGui::BeginTabBar("DocTabs")) {
            if (ImGui::BeginTabItem("Attack Skills")) {
                static ryml::ConstNodeRef selected_node = nullptr;
                ImGui::BeginChild("left pane", ImVec2(300, 0), true);
                for (ryml::ConstNodeRef label_node : custom_labels.tree.rootref()) {
                    // Selectable object for every string in the array
                    if (!label_node.has_child("name")) {
                        // Nothing to render if there's no name...
                        continue;
                    }

                    // We need to make a null-terminated std::string because
                    // ImGui::Selectable doesn't understand the string container
                    // used by ryml.
                    const c4::basic_substring name_substr = label_node["name"].val();
                    const std::string name = std::string(name_substr.data(), name_substr.size());
                    if (ImGui::Selectable(name.data()) || selected_node.invalid()) {
                        selected_node = label_node;
                    }
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("Doc Text", ImVec2(600, 0), false);

                // The YAML might be missing a field, so we have placeholders
                std::string_view name_view = "[Missing name]";
                std::string_view docs_view = "[Missing docs]";

                // Trying to get a field that doesn't exist will crash
                // We have to convert to the stdlib equivalent of C4's substring
                // type, because you can only append the stdlib version to a std::string.
                if (selected_node.has_child("name")) {
                    const c4::basic_substring temp = selected_node["name"].val();
                    name_view = std::string_view(temp.data(), temp.size());
                }
                if (selected_node.has_child("docs")) {
                    const c4::basic_substring temp = selected_node["docs"].val();
                    docs_view = std::string_view(temp.data(), temp.size());
                } else if (selected_node.has_child("desc")) {
                    // Docs are missing, use the description as a backup
                    const c4::basic_substring temp = selected_node["desc"].val();
                    docs_view = std::string_view(temp.data(), temp.size());
                }

                // Renders the item name as an H1, then the description on a new
                // line. For some reason this can't all happen in 1 declaration.
                std::string md = "# ";
                md.append(name_view);
                md += '\n';
                md.append(docs_view);
                ImGui::Markdown(md.c_str(), md.length(), mdConfig);

                if (selected_node.has_child("conditions")) {
                    std::string conditions = "# Conditions\n All of these conditions must be met for this to be shown in the editor:\n\n";
                    for (ryml::ConstNodeRef cond : selected_node["conditions"]) {
                        const c4::basic_substring comparison = cond["comparison"].val();
                        const c4::basic_substring val = cond["val"].val();
                        const c4::basic_substring pos_str = cond["pos"].val();
                        u8 pos = 0;
                        cond["pos"]  >> pos;

                        const c4::basic_substring target_name = custom_labels.labels[pos].name;

                        // All of this gives a string that looks something like:
                        // "Skill ID" [pos 0xE] == 400
                        conditions += "\"";
                        conditions += std::string_view(target_name.str, target_name.len);
                        conditions += "\" [pos ";
                        conditions += std::string_view(pos_str.str, pos_str.len);
                        conditions += "] ";

                        conditions += std::string_view(comparison.str, comparison.len);
                        conditions += " ";
                        conditions += std::string_view(val.str, val.len);
                        conditions += '\n';
                    }

                    ImGui::Markdown(conditions.c_str(), conditions.length(), mdConfig);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Skill Editor")) {
                static uint16_t select_idx = 0;
                ImGui::BeginChild("left pane", ImVec2(350, 0), true);
                for (unsigned short i = 0; i < (unsigned short)IM_ARRAYSIZE(DocumentationProgramLabels); i++) {
                    // Selectable object for every string in the array
                    if (ImGui::Selectable(DocumentationProgramLabels[i])) {
                        select_idx = i;
                    }
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("Doc Text", ImVec2(800, 0), false);
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


    if (text_edit) {
        ImGui::Begin("Skill Text Editor", &text_edit);

        // The input field modifies a std::string, so there's no length limit.
        ImGui::InputText("Skill Name", &current_name);
        ImGui::InputText("Skill Description", &current_desc);

        static u16 text_id = 0;
        uint16_t cache = text_id; // Previously selected skill ID
        text_id = p.gstorage->skill_array[ID - 1].SkillTextID;

        if (ImGui::Button("Reload") || cache != text_id) {
            skill_text text = get_skill_text(p, text_id);
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