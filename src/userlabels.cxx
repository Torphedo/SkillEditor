#include <optional>
#include <common/logging.h>
#include "userlabels.hxx"

typedef struct {
    const char* text;
    u32 type;
}type_table_entry;

// Lookup tables to replace a bunch of if statements in other functions
type_table_entry data_type_table[] = {
    {"S8",  ImGuiDataType_S8},
    {"U8",  ImGuiDataType_U8},
    {"S16", ImGuiDataType_S16},
    {"U16", ImGuiDataType_U16},
    {"S32", ImGuiDataType_S32},
    {"U32", ImGuiDataType_U32},
    {"S64", ImGuiDataType_S64},
    {"U64", ImGuiDataType_U64},
};

type_table_entry comp_type_table[] = {
    {"==", EQUAL},
    {"<",  LESS_THAN},
    {"<=", LESS_THAN_OR_EQUAL},
    {">",  GREATER_THAN},
    {">=", GREATER_THAN_OR_EQUAL},
};

ImGuiDataType get_type(c4::basic_substring<const char> type_str) {
    for (const type_table_entry& entry : data_type_table) {
        if (strncmp(type_str.str, entry.text, type_str.len) == 0) {
            return (ImGuiDataType)entry.type;
        }
    }

    // We can't print here because the documentation window calls this every
    // frame to show info about the data type, and it'd spam the log
    /*
    LOG_MSG(warning, "Data type '%.*s' is invalid, S8 will be assumed.\n", (int)type_str.len, type_str.str);
    LOG_MSG(info, "The valid data types are: ");
    for (const type_table_entry& entry : data_type_table) {
        printf("'%s' ", entry.text);
    }
    printf("\n");
    */

    // Nothing worked, we'll assume S8
    return ImGuiDataType_S8;
}

compare_t parse_comparison(c4::basic_substring<const char> str) {
    for (const type_table_entry& entry : comp_type_table) {
        if (strncmp(str.str, entry.text, str.len) == 0) {
            return (compare_t)entry.type;
        }
    }
    return COMPARISON_INVALID;
}

userlabel parse_label(ryml::ConstNodeRef label_node, u8& pos) {
    userlabel out = {};

    const bool unusable = !label_node.has_child("pos") || !label_node.has_child("type");
    if (unusable) {
        // Try to find out the name for error reporting
        c4::basic_substring name = "[name missing]";
        if (label_node.has_child("name")) {
            name = label_node["name"].val();
        }

        // If there's no position, we don't know where it's supposed to go.
        // If there's no type, we don't know how to interpret the data.
        printf("I don't know what to do with your label \"%.*s\", because it's missing a position or data type.\n", (int)name.len, name.str);
        return out;
    }

    // The label is usable!
    label_node["pos"] >> pos;

    // Try to parse all fields
    if (label_node.has_child("name")) {
        out.name = label_node["name"].val();
        // The label is only suitable to display if it has a name
        out.exists = true;
    }
    else if (label_node.num_children() > 2) {
        // Not having a name isn't necessarily an error, you might want to have
        // a hidden "label" that only sets a field's data type so it can be
        // checked by a conditional label. But if there's extra data attached,
        // the missing name is probably a mistake.

        // We print the original string used for the position, so it looks
        // exactly like what the user wrote (whether hexadecimal or decimal format)
        const c4::basic_substring pos_str = label_node["pos"].val();
        printf("It seems like you wanted to use a label with position %.*s, but I can't display it because it has no name.\n", (int)pos_str.len, pos_str.str);
    }

    if (label_node.has_child("desc")) {
        out.desc = label_node["desc"].val();
    }

    if (label_node.has_child("docs")) {
        out.docs = label_node["docs"].val();
    }

    if (label_node.has_child("type")) {
        // Convert type string to usable ImGui value
        out.type = get_type(label_node["type"].val());
    }

    // Deserialize non-string fields
    if (label_node.has_child("limit_low")) {
        label_node["limit_low"] >> out.limit_low;
    }

    if (label_node.has_child("limit_high")) {
        label_node["limit_high"] >> out.limit_high;
    }

    if (label_node.has_child("slider")) {
        label_node["slider"] >> out.slider;
    }

    return out;
}

void user_config::update_unconditional_labels() {
    for (ryml::ConstNodeRef label_node : tree.rootref()) {
        u8 pos = 0;
        const userlabel label = parse_label(label_node, pos);
        if (!label.exists) {
            // Some parsing error (probably missing position/type), skip it.
            continue;
        }
        if (label_node.has_child("conditions")) {
            if (label_node["conditions"].num_children() > 0) {
                // This label depends on the value(s) of some other label(s),
                // which might not have been parsed yet. We can't know how to
                // interpret the value until we parse its data type from a
                // label, so we skip the conditional label in this pass.
                continue;
            }
        }

        // The label is valid, save it to the table
        this->labels[pos] = label;
    }
}

bool do_comparison(const void* comp_target, ImGuiDataType target_type, s64 val, compare_t comparison) {
    s64 compare_to = 0;

    switch (target_type) {
        case ImGuiDataType_U8:
            compare_to = *((u8*)comp_target);
            break;
        case ImGuiDataType_S8:
            compare_to = *((s8*)comp_target);
            break;
        case ImGuiDataType_U16:
            compare_to = *((u16*)comp_target);
            break;
        case ImGuiDataType_S16:
            compare_to = *((s16*)comp_target);
            break;
        case ImGuiDataType_U32:
            compare_to = *((u32*)comp_target);
            break;
        case ImGuiDataType_S32:
            compare_to = *((s32*)comp_target);
            break;
        case ImGuiDataType_U64:
            // Numbers >INT64_MAX might go negative here...
            compare_to = *((u64*)comp_target);
            break;
        case ImGuiDataType_S64:
            compare_to = *((s64*)comp_target);
            break;
        default:
            printf("Unsupported data type %d!\n", target_type);
            return false;
    }

    switch (comparison) {
        case EQUAL:
            return compare_to == val;
        case LESS_THAN:
            return compare_to < val;
        case LESS_THAN_OR_EQUAL:
            return compare_to <= val;
        case GREATER_THAN:
            return compare_to > val;
        case GREATER_THAN_OR_EQUAL:
            return compare_to >= val;
        default:
            printf("Invalid comparison type!\n");
            return false;
    }
}

void user_config::update_conditional_labels(skill_t skill) {
    for (ryml::ConstNodeRef label_node : tree.rootref()) {
        u8 label_pos = 0;
        const userlabel label = parse_label(label_node, label_pos);
        if (!label.exists) {
            // Some parsing error (probably missing position/type), skip it.
            continue;
        }
        if (!label_node.has_child("conditions")) {
            // We're only parsing conditional labels in this pass.
            continue;
        }

        bool conditions_passed = true;
        ryml::ConstNodeRef all_conditions = label_node["conditions"];
        for (u32 i = 0; i < all_conditions.num_children(); i++) {
            const ryml::ConstNodeRef cond = all_conditions.child(i);
            const bool unusable = !cond.has_child("pos") || !cond.has_child("val") || !cond.has_child("comparison");
            if (unusable) {
                printf("I can't make sense of condition #%d for your label \"%.*s\", because it's missing a position, value, or comparison type. The label won't be used.\n", i, (int)label.name.len, label.name.str);
                conditions_passed = false;
                break;
            }

            // From here on we know all the fields are valid
            s64 val = 0;
            u8 pos = 0;
            const compare_t comparison = parse_comparison(cond["comparison"].val());

            // Deserialize our values
            cond["val"] >> val;
            cond["pos"] >> pos;

            // Interpret the data at [pos] as the appropriate data type

            // Find position & type of the data
            const void* comp_target = (void*)(((u8*)&skill) + pos);
            const ImGuiDataType target_type = this->labels[pos].type;

            // Doing completely generic comparisons has a lot of cases to cover,
            // so that's its own function.
            const bool success = do_comparison(comp_target, target_type, val, comparison);

            // All conditions must pass for the label to be used
            conditions_passed &= success;
        }

        if (conditions_passed) {
            this->labels[label_pos] = label;
        }
    }
}

user_config::user_config(char* yaml) {
    // ryml needs a substring wrapper object to work with the text.
    c4::basic_substring<char> yaml_substr = c4::basic_substring(yaml, strlen(yaml));
    tree = ryml::parse_in_place(yaml_substr);

    this->update_unconditional_labels();
}

std::optional<u32> user_config::render_editor(skill_t* skill, bool limitless) {
    u8* buf = (u8*) skill;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    ImGui::AlignTextToFramePadding();

    std::optional<u32> selected_item;
    if (ImGui::BeginTable("split1", 2, ImGuiTableFlags_NoSavedSettings)) {
        for (size_t i = 0; i < IM_ARRAYSIZE(this->labels); i++) {
            const userlabel label = this->labels[i];
            if (!label.exists) {
                // Skip empty labels
                continue;
            }

            // Creates 2-column view
            ImGui::TableNextColumn();

            char label_buf[2048] = {0};
            snprintf(label_buf, sizeof(label_buf), "%.*s", (int) label.name.len, label.name.str);
            // We make the input boxes narrow to make sure the text doesn't get cut off
            ImGui::SetNextItemWidth(200);


            if (label.limit_low != label.limit_high && label.slider && !limitless) {
                ImGui::SliderScalar(label_buf, label.type, &buf[i], &label.limit_low, &label.limit_high);
            } else {
                const s64 step = 1;
                ImGui::InputScalar(label_buf, label.type, &buf[i], &step);
            }

            if (ImGui::IsItemHovered() && label.desc.len > 0) {
                ImGui::SetTooltip("%.*s", (int) label.desc.len, label.desc.str);
            }

            char popup_id[0x20] = {0};
            sprintf(popup_id, "%lld", i);
            if (ImGui::BeginPopupContextItem(popup_id)) {
                bool selected = false;
                ImGui::MenuItem("Open documentation", "", &selected);
                if (selected) {
                    selected_item = (u32)i;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        ImGui::EndTable();
    }
    ImGui::PopStyleVar();

    return selected_item;
}