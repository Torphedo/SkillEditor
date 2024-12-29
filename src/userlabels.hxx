#pragma once
#include <ryml.hpp>
#include <imgui.h>

#include <common/int.h>
#include "structs.h"

typedef enum : u8 {
    EQUAL,
    LESS_THAN,
    LESS_THAN_OR_EQUAL,
    GREATER_THAN,
    GREATER_THAN_OR_EQUAL,
    COMPARISON_INVALID,
}compare_t;

struct userlabel {
    // You can only print these with the "%.*s" specifier
    c4::basic_substring<const char> name;
    c4::basic_substring<const char> desc;
    c4::basic_substring<const char> docs;

    // Lower/upper bounds, if appropriate.
    s64 limit_low = 0;
    s64 limit_high = 0;
    ImGuiDataType type = ImGuiDataType_S8;
    bool slider = false;

    // A label only "exists" if it was parsed in from the config file.
    bool exists = false;
};

struct user_config {
    userlabel labels[sizeof(skill_t)] = {};
    ryml::Tree tree;

    user_config() = default;
    user_config(char* yaml_data);
    void update_unconditional_labels();
    void update_conditional_labels(skill_t skill);
    void render_editor(skill_t* skill, bool limitless);
};