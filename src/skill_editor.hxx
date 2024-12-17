#pragma once
#include <string>

#include "userlabels.hxx"
#include <imgui_hex_editor.h>
#include <imgui_markdown.h>
#include "memory_editing.hxx"

struct editor {
    pd_meta p = {};
    u16 ID = 1;
    bool NewSkillPack = false;
    bool HexEditor = false;
    bool AttackSkillEditor = false;
    bool Documentation = false;
    bool IDSelection = false;
    bool text_edit = false;
    bool text_prompt = false;
    bool limitless = false;
    bool decimal_id = false;

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
    void AtkSkillWindow(skill_t* skill);
};