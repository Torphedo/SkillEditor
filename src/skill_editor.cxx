#include <imgui.h>
#include <imgui_internal.h> // For messing with the viewport for menu bar
#include <imgui/misc/cpp/imgui_stdlib.h> // For std::string input fields
#include <imgui_markdown.h>
#include <nfd.h>

#include "skill_editor.hxx"
#include "mods.hxx"
#include "text.h"
#include "common/logging.h"
#include "nfde_wrapper.hxx"

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
    if (cfg_data == nullptr) {
        // Pause so the user can see the error message
        system("pause");
    } else {
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
        const bool gstorage_changed = flush_to_pd(p, use_vanilla_version);

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
    bool load_from_disk = control && shift && ImGui::IsKeyPressed(ImGuiKey_L, false);
    bool save_as = control && shift && ImGui::IsKeyPressed(ImGuiKey_S, false);
    bool save_normal = control && ImGui::IsKeyPressed(ImGuiKey_S, false);
    bool toggle_freeze_game = ImGui::IsKeyPressed(ImGuiKey_F4, false);

    if (ImGui::BeginViewportSideBar("MenuBar", viewport, ImGuiDir_Up, height, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                new_skill_pack |= ImGui::MenuItem("New Skill Pack", "Ctrl-N");

                if (ImGui::BeginMenu("Open")) {
                    load_from_mem |= ImGui::MenuItem("Skill (From Memory)", "Ctrl-L");
                    load_from_disk |= ImGui::MenuItem("Skill (From file)", "Ctrl-Shift-L");
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
                ImGui::MenuItem("Skill Hex Editor", nullptr, &hex_edit.Open);
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
                ImGui::MenuItem("Use vanilla version number", nullptr, &use_vanilla_version);
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
        std::vector<std::string> paths;
        const nfdu8filteritem_t filters[] = { { "Skill File", "*" } };
        auto res = NFD_OpenDialogMultipleAutoFree(paths, filters, ARRAY_SIZE(filters), nullptr);
        if (res == NFD_OKAY) {
            char* out_path = nullptr;
            if (!skill_select(&out_path)) {
                LOG_MSG(info, "Skill pack creation cancelled.\n");
            } else {
                save_skill_pack(out_path, paths);
            }
            if (out_path != nullptr) {
                NFD_FreePathU8(out_path);
            }
        } else {
            LOG_MSG(info, "Skill selection cancelled.\n");
        }
    }

    if (load_from_disk) {
        if (!game_available) {
            printf("Can't access game's skill data in memory, cancelling skill pack install.\n");
        } else {
            // Open a multiple file open dialog
            std::vector<std::string> paths;
            const nfdu8filteritem_t filters[] = { { "Skill File", "sp3" } };
            auto res = NFD_OpenDialogMultipleAutoFree(paths, filters, ARRAY_SIZE(filters), nullptr);
            if (res != NFD_OKAY) {
                LOG_MSG(info, "Skill pack selection canceled.\n");
            } else {
                install_mod(p, paths.data(), paths.size());
                AttackSkillEditor = true; // Open the Attack Skill Editor window
            }
        }
    }
    else if (load_from_mem) {
        if (game_available) {
            IDSelection = true;
        }
    }

    if (toggle_freeze_game) {
        toggle_game_pause(p);
    }

    static char* filepath = nullptr;
    // Saving
    if (save_as || save_normal) {
        if (save_normal && filepath != nullptr) {
            // We already have a filepath, don't need to prompt the user
            text_prompt = true;
        } else {
            if (filepath != nullptr) {
                NFD_FreePathU8(filepath);
                filepath = nullptr;
            }
            // Only open prompt if a file was chosen
            text_prompt = skill_select(&filepath);
        }
    }

    if (text_prompt && filepath != nullptr) {
        ImGui::Begin("Save text to the skill file?");
        const bool no = ImGui::Button("No");
        ImGui::SameLine();
        const bool yes = ImGui::Button("Yes");

        // Proceed if either is pressed, but only save text if they said yes
        if (yes || no) {
            save_skill_to_file(filepath, p, ID, yes);
            text_prompt = false;
            // We don't free the filepath here because we save to the same file
            // again unless the user chooses a new one.
        }
        ImGui::End();

    }

    if (IDSelection) {
        // Temporary storage to hold an ID before actually updating the selected ID
        static s32 temp_id = 0;
        static bool just_opened = true;
        ImGui::Begin("Input a skill ID: ", &IDSelection);
        ImGui::Checkbox("Hexadecimal", &hexadecimal_ids);
        if (just_opened) {
            ImGui::SetKeyboardFocusHere();
            just_opened = false;
        }
        const int flags = ImGuiInputTextFlags_CharsHexadecimal * hexadecimal_ids;
        ImGui::InputInt("ID", &temp_id, 1, 1, flags);
        // InputScalar doesn't support EnterReturnsTrue for some reason
        const bool enter_pressed = ImGui::IsItemDeactivatedAfterEdit();

        const bool open_pressed = ImGui::Button("Open");
        if (open_pressed || enter_pressed) {
            ID = temp_id;
            printf("Loaded skill with ID %d\n", ID);
            IDSelection = false;      // Close this window
            just_opened = true;
            AttackSkillEditor = true; // Opens the Attack Skill Editor window
        }
        ImGui::End();
    }

    if (hex_edit.Open) {
        hex_edit.OptShowAscii = false;
        hex_edit.DrawWindow("Hex Editor", &p.gstorage->skill_array[ID - 1], 144);
    }

    if (AttackSkillEditor) {
        // Render the editor w/ user-controlled labels
        if (ImGui::Begin("Skill Editor", &this->AttackSkillEditor)) {
            std::optional<u32> selected_item = this->custom_labels.render_editor(this->cur_skill(), limitless);
            if (selected_item.has_value()) {
                Documentation = true;
                for (ryml::ConstNodeRef label_node : custom_labels.tree.rootref()) {
                    if (!label_node.has_child("name")) {
                        continue;
                    }

                    if (label_node["name"].val() == custom_labels.labels[selected_item.value()].name) {
                        selected_node = label_node;
                    }
                }
            }
        }
        ImGui::End();
    }

    if (Documentation) {
        ImGui::SetNextWindowSize(ImVec2(850, 650), ImGuiCond_FirstUseEver);
        ImGui::Begin("Documentation", &Documentation);

        if (ImGui::BeginTabBar("DocTabs")) {
            if (ImGui::BeginTabItem("Attack Skills")) {
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

                u8 pos = 0;
                const userlabel parsed_label = parse_label(selected_node, pos);
                char data_info[0x40] = {0};
                const ImGuiDataTypeInfo* type_info = ImGui::DataTypeGetInfo(parsed_label.type);
                snprintf(data_info, sizeof(data_info) - 1, "\nType: %s (%llu bytes) @ offset 0x%X\n", type_info->Name, type_info->Size, pos);

                ImGui::Markdown(data_info, strlen(data_info), mdConfig);

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
            skill_text text = {current_name.c_str(), current_desc.c_str()};
            save_skill_text(p, text, text_id);
        }
        ImGui::End();
    }
    return 0;
}