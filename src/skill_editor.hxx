#pragma once
#include <string>

#include "userlabels.hxx"
#include <imgui_hex_editor.h>
#include <imgui_markdown.h>
#include "remote_pd.hxx"

struct editor {
    pd_meta p = {};
    u16 ID = 1;
    bool NewSkillPack = false;
    bool AttackSkillEditor = true;
    bool Documentation = true;
    bool IDSelection = false;
    bool text_edit = true;
    bool text_prompt = false;

    // Whether to remove all limits on input fields
    bool limitless = false;

    // Whether to display IDs in ID selection window in hex format
    bool hexadecimal_ids = false;

    char* cfg_yaml = nullptr;
    user_config custom_labels;
    MemoryEditor hex_edit;
    ImGui::MarkdownConfig mdConfig;

    // Name & description being edited in text edit box
    std::string current_name;
    std::string current_desc;

    editor();
    ~editor();
    int draw();
    skill_t* cur_skill();
};