#include "userlabels.hxx"

ImGuiDataType get_type(c4::basic_substring<const char> type_str) {
    // C4's substring wrapper uses == operator for string compare
    if (type_str == "S8") {
        return ImGuiDataType_S8;
    }
    else if (type_str == "U8") {
        return ImGuiDataType_U8;
    }
    else if (type_str == "U16") {
        return ImGuiDataType_U16;
    }
    else if (type_str == "S16") {
        return ImGuiDataType_S16;
    }
    else if (type_str == "U32") {
        return ImGuiDataType_U32;
    }
    else if (type_str == "S32") {
        return ImGuiDataType_S32;
    }
    else if (type_str == "U64") {
        return ImGuiDataType_U64;
    }
    else if (type_str == "S64") {
        return ImGuiDataType_U64;
    } else {
        // Nothing worked, we'll assume S8
        return ImGuiDataType_S8;
    }
}

user_config::user_config(char* yaml) {
    // ryml needs a substring wrapper object to work with the text.
    c4::basic_substring<char> yaml_substr = c4::basic_substring(yaml, strlen(yaml));
    const ryml::Tree tree = ryml::parse_in_place(yaml_substr);

    // Loop over all labels
    for (ryml::ConstNodeRef label_node : tree.rootref()) {
        // Parse into label structure
        if (!label_node.has_child("pos")) {
            // If there's no position, we don't know where it's supposed to go.
            const char* name = "[name missing]";
            if (label_node.has_child("name")) {
                name = label_node["name"].val().str;
            }
            printf("Label \"%s\" is missing a position, it can't be loaded!\n", name);
            continue;
        }

        // The label is parsable!
        u8 pos = 0;
        label_node["pos"] >> pos;
        this->labels[pos].exists = true;

        // Try to parse all fields
        if (label_node.has_child("name")) {
            this->labels[pos].name = label_node["name"].val();
        }

        if (label_node.has_child("desc")) {
            this->labels[pos].desc = label_node["desc"].val();
        }

        if (label_node.has_child("docs")) {
            this->labels[pos].docs = label_node["docs"].val();
        }

        if (label_node.has_child("type")) {
            // Convert type string to usable ImGui value
            this->labels[pos].type = get_type(label_node["type"].val());
        }

        if (label_node.has_child("limit_low")) {
            label_node["limit_low"] >> this->labels[pos].limit_low;
        }

        if (label_node.has_child("limit_high")) {
            label_node["limit_high"] >> this->labels[pos].limit_high;
        }
    }
}

void user_config::render_editor(atkskill* skill) {
    u8* buf = (u8*) skill;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    ImGui::AlignTextToFramePadding();

    if (ImGui::BeginTable("split1", 2, ImGuiTableFlags_NoSavedSettings)) {
        for (size_t i = 0; i < IM_ARRAYSIZE(this->labels); i++) {
            const userlabel label = this->labels[i];
            if (!label.exists) {
                // Skip empty labels
                continue;
            }

            // Creates 2-column view
            ImGui::TableNextColumn();

            const u8 step = 1;
            const u8 fast_step = 5;
            char label_buf[2048] = {0};
            snprintf(label_buf, sizeof(label_buf), "%.*s", (int) label.name.len, label.name.str);
            // We make the input boxes narrow to make sure the text doesn't get cut off
            ImGui::SetNextItemWidth(200);
            ImGui::InputScalar(label_buf, label.type, &buf[i], &step);

            if (ImGui::IsItemHovered() && label.desc.len > 0) {
                ImGui::SetTooltip("%.*s", (int) label.desc.len, label.desc.str);
            }
        }

        ImGui::EndTable();
    }

    ImGui::PopStyleVar();
}