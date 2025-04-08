#pragma once
#include <string>

#include "userlabels.hxx"
#include <imgui_hex_editor.h>
#include <imgui_markdown.h>
#include "remote_pd.hxx"

struct editor {
    pd_meta p = {};

    u16 ID = 1;

    char* cfg_yaml = nullptr;
    user_config custom_labels;
    ryml::ConstNodeRef selected_node = nullptr;

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