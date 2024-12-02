#include <iostream>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui_hex_editor.h>
#include <imgui_markdown.h>

#include "../winAPI.hxx"
#include "program-ui.hxx"
#include "../skill_io.h"
#include "../text.hxx"
#include "../memory-editing.hxx"

struct {
    bool NewSkillPack;
    bool HexEditor;
    bool AttackSkillEditor;
    bool Documentation;
    bool IDSelection;
    bool text_edit;
    bool text_prompt;
    bool limitless;
    bool decimal_id;
    MemoryEditor hex_edit;

    // Name & description being edited in text edit box
    std::string current_name;
    std::string current_desc;
}ui_state;

unsigned short ID = 1;

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

void AtkSkillWindow(atkskill* skill);
static void Markdown(const std::string& markdown_); // Markdown function prototype

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

int ProgramUI(pd_meta* p) {
    ImGuiViewportP* viewport = (ImGuiViewportP*)ImGui::GetMainViewport();
    float height = ImGui::GetFrameHeight();

    ImGui::DockSpaceOverViewport(); // Enable docking
    const bool game_available = handle_still_valid(p->h);
    const bool game_running = is_running();

    if (!game_available) {
        update_process(p, false);
    }

    if (game_running && game_available) {
        flush_to_pd(*p);
    }

    if (ImGui::BeginViewportSideBar("MenuBar", viewport, ImGuiDir_Up, height, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Skill Pack", "Ctrl-N")) {
                    if (SUCCEEDED(file_multiple_select_dialog())) {
                        ui_state.NewSkillPack = true;
                    }
                    else {
                        printf("Skill selection canceled.\n");
                    }
                }
                if (ImGui::BeginMenu("Open")) {
                    if (ImGui::MenuItem("Skill (From Memory)", "Ctrl-L")) {
                        if (game_available) {
                            ui_state.IDSelection = true;
                        }
                    }
                    if (ImGui::MenuItem("Skill File")) {
                        ID = load_attack_skill(*p, ID);
                        ui_state.AttackSkillEditor = true; // Open the Attack Skill Editor window
                    }
                    if (ImGui::MenuItem("Install Skill Pack")) {
                        if (game_available) {
                            if (SUCCEEDED(file_multiple_select_dialog())) { // Open a multiple file open dialog
                                install_mod(*p);
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
                    ui_state.text_prompt = true;
                }
                if (ImGui::MenuItem("Save As", "Ctrl + S")) {
                    skill_select();
                    ui_state.text_prompt = true;
                }

                if (ImGui::MenuItem("Exit", "Alt + F4")) {
                    return 1;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window")) {
                ImGui::MenuItem("Attack Skill Editor", nullptr, &ui_state.AttackSkillEditor);
                if (ImGui::MenuItem("Skill Hex Editor")) {
                    if (game_available) {
                        ui_state.HexEditor = !ui_state.HexEditor;
                    }
                }
                ImGui::MenuItem("Documentation", nullptr, &ui_state.Documentation);
                if (ImGui::MenuItem("Text Edit", nullptr, &ui_state.text_edit)) {
                    update_process(p, false); // Refresh skill data address & game handle

                    skill_text text = load_skill_text(*p, ID);
                    ui_state.current_name = text.name;
                    ui_state.current_desc = text.desc;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options")) {
                if (ImGui::MenuItem("Freeze/Unfreeze Phantom Dust", "F4")) {
                    toggle_game_pause(*p);
                }
                ImGui::Checkbox("Input Box Limits", &ui_state.limitless);
                ImGui::EndMenu();
            }
            
            if (!handle_still_valid(p->h)) {
                ImGui::SameLine(viewport->Size.x - 300);
                ImGui::TextColored({ 255, 0, 0, 255 }, "No Phantom Dust instance detected!");
            }
            else {
                ImGui::SameLine(viewport->Size.x - 15 - 575);
                if (p->h == INVALID_HANDLE_VALUE || p->h == NULL) {
                    ImGui::TextColored({ 255, 0, 0, 255 }, "No handle to process!");
                }
                else if (!game_available) {
                    ImGui::TextColored({ 255, 0, 0, 255 }, "Can't read from process!");
                }
                ImGui::SameLine(viewport->Size.x - 425);

                if (ImGui::Button("Refresh process")) {
                    printf("Refreshing process info & skill/text data... \n");
                    update_process(p, true);
                }

                ImGui::Text("Phantom Dust Process ID:");
                ImGui::TextColored({ 0, 255, 0, 255 }, "%u", p->pid);
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
            ui_state.IDSelection = true;
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F4)) {
        toggle_game_pause(*p);
    }
    // Save
    if (ImGui::IsKeyPressed(ImGuiKey_S)) {
        // Save As
        if (control) {
            if (skill_select()) {
                ui_state.text_prompt = true;
            }
        }
        // Save
        else if (shift) {
            ui_state.text_prompt = true;
        }
    }
    if (control && ImGui::IsKeyPressed(ImGuiKey_N, false) && !ui_state.NewSkillPack) {
        if (SUCCEEDED(file_multiple_select_dialog())) {
            ui_state.NewSkillPack = true;
        }
    }

    if (ui_state.text_prompt) {
        // Stupid and convoluted. If we don't use OpenPopup(), it will create it with the default
        // frame ("Debug" label). But it breaks if called from a menu item, so we need this.
        ImGui::OpenPopup("save_text_prompt");
    }

    if (ImGui::BeginPopup("save_text_prompt")) {
        ImGui::Text("Save text to the skill file?");
        if (ImGui::Button("No")) {
            save_skill_to_file(*p, ID, false);
            ui_state.text_prompt = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Yes")) {
            save_skill_to_file(*p, ID, true);
            ui_state.text_prompt = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ui_state.IDSelection) {
        // Temporary storage to hold an ID before actually updating the selected ID
        static s32 temp_id = 0;
        static bool hex = false;

        ImGui::Begin("Input a skill ID: ");
        ImGui::Checkbox("Hexidecimal", &hex);
        ImGui::InputInt("ID", &temp_id, 1, 1, ImGuiInputTextFlags_CharsHexadecimal * hex);
        if (ImGui::Button("Open")) {
            ID = temp_id;
            printf("Loaded skill with ID %d\n", ID);
            ui_state.IDSelection = false;      // Close this window
            ui_state.AttackSkillEditor = true; // Opens the Attack Skill Editor window
        }
        ImGui::End();
    }

    if (ui_state.HexEditor) {
        ui_state.hex_edit.OptShowAscii = false;
        ui_state.hex_edit.DrawWindow("Hex Editor", &p->gstorage->skill_array[ID - 1], 144);
    }

    if (ui_state.AttackSkillEditor) {
        // This window is way too long to inline here
        atkskill* skill = &p->gstorage->skill_array[ID - 1];
        AtkSkillWindow(skill);
    }

    if (ui_state.Documentation) {
        ImGui::SetNextWindowSize(ImVec2(850, 650), ImGuiCond_FirstUseEver);
        ImGui::Begin("Documentation", &ui_state.Documentation);

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
                Markdown(md);

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
                Markdown(md);

                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }


    if (ui_state.NewSkillPack) {
        static char packname_internal[32] = {0}; // Mod name that will be stored in the binary
        ImGui::Begin("Enter a name for your skill pack: ");
        ImGui::InputText("Skill Pack Name", packname_internal, 32);
        if (ImGui::Button("Save")) {
            save_skill_pack(packname_internal);
            ui_state.NewSkillPack = false;
        }
        ImGui::End();
    }

    if (ui_state.text_edit) {
        ImGui::Begin("Skill Text Editor", &ui_state.text_edit);

        // Allow text input, limited to the size of the original text
        ImGui::InputText("Skill Name", &ui_state.current_name);
        ImGui::InputText("Skill Description", &ui_state.current_desc);

        static uint16_t text_id = 0;
        uint16_t cache = text_id; // Previously selected skill ID
        text_id = p->gstorage->skill_array[ID - 1].SkillTextID + 1;

        if (ImGui::Button("Reload") || cache != text_id) {
            skill_text text = load_skill_text(*p, text_id);
            ui_state.current_name = text.name;
            ui_state.current_desc = text.desc;
        }

        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            skill_text text = {ui_state.current_name, ui_state.current_desc};
            save_skill_text(*p, text, text_id);
        }
        ImGui::End();
    }
    return 0;
}

void AtkSkillWindow(atkskill* skill) {
    if (!ImGui::Begin("Attack Skill Editor", &ui_state.AttackSkillEditor)) {
        ImGui::End();
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    ImGui::AlignTextToFramePadding();

    if (ImGui::BeginTable("split1", 2, ImGuiTableFlags_NoSavedSettings)) {

        ImGui::TableNextColumn();
        InputShort("Skill Text ID", &skill->SkillTextID, 1);
        Tooltip("The skill ID from which to get the skill's name,\ndescription, etc.");
        InputShort("Register ID", &skill->RegisterID, 1);
        Tooltip("The \"register\" / \"library\" of skills this skill\nwill belong to.");
        InputShort("Skill ID", &skill->SkillID, 1);
        Tooltip("The skill's internal ID. This will determine what\nskill will be overwritten. This internal ID has no\nrelation to the IDs seen in-game.");

        ImGui::SetNextItemWidth(200);
        if (ui_state.limitless) {
            InputShort("Rarity", &skill->RarityStars, 1);
        }
        else {
            ImGui::SliderShort("Rarity", &skill->RarityStars, 1, 5, nullptr, 0);
        }
        Tooltip("The skill's in-game rarity, displayed as stars.\n0 -> 1 star, 1 -> 2 stars, etc.");

        InputShort("Sound File ID", &skill->SoundFileID, 1);
        ImGui::TableNextColumn();

        ImGui::SetNextItemWidth(200);
        u16* capsule_type = &skill->CapsuleType;
        if (ui_state.limitless) {
            InputShort("Capsule Type", capsule_type, 1);
        }
        else {
            static const char *elems_names[7] = {"Aura", "Attack", "Defense", "Erase", "Special", "Status",
                                                 "Environmental"};
            if (*capsule_type > 6) {
                ImGui::SliderShort("Capsule Type", (uint16_t *) capsule_type, 0, 32, nullptr, 0);
            } else {
                ImGui::SliderShort("Capsule Type", (uint16_t *) capsule_type, 0, 6, elems_names[*capsule_type], 0);
            }
        }

        ImGui::TableNextColumn();

        InputShort("School ID", &skill->SchoolID, 1);
        Tooltip("The skill's school. (Nature, Optical, Ki, etc.)");
        InputShort("Animation Profile (Ground)", &skill->AnimationProfileGround, 1);
        Tooltip("This controls the player's skeletal animation, the number\nof projectiles fired, particle effects used, and much more.\nThis profile is used when the skill is cast on the ground.");
        InputShort("Animation Profile (Air)", &skill->AnimationProfileAir, 1);
        Tooltip("This controls the player's skeletal animation, the number\nof projectiles fired, particle effects used, and much more.\nThis profile is used when the skill is cast in the air.");
        InputShort("Multi Press 1", &skill->MultiPress1, 1);
        ImGui::TableNextColumn();
        InputShort("Multi Press 2", &skill->MultiPress2, 1);
        ImGui::TableNextColumn();
        InputShort("Double Skill 1", &skill->DoubleSkill1, 1);
        ImGui::TableNextColumn();
        InputShort("Double Skill 2", &skill->DoubleSkill2, 1);
        ImGui::TableNextColumn();
        InputShort("After Hit SFX", &skill->PostHitSFX, 1);
        ImGui::TableNextColumn();
        InputShort("Start Up SFX", &skill->StartUpSFX, 1);
        Tooltip("The sound effect ID to be played when the skill\nis winding up / charging.");
        InputShort("Collision SFX", &skill->CollisionSFX, 1);
        Tooltip("The sound effect ID to be played when the skill\ncollides with something.");
        InputShort("Aura Cost", &skill->Cost, 1);
        Tooltip("The amount of Aura the skill costs.");
        InputShort("Cost Effect", &skill->CostEffect, 1);
        Tooltip("Additional special costs.\n0 = None\n1 = Reset Aura\n2 = Require Max Aura");
        InputShort("Additional Aura Cost", &skill->ExtraCost, 1);
        Tooltip("Adds to the base Aura cost.");
        InputShort("Health Penalty", &skill->HealthCost, 1);
        Tooltip("The amount of health to be taken from the user\nwhen the skill is cast.");
        InputShort("# of Uses", &skill->SkillUses, 1);
        Tooltip("How many times the skill can be used. Set this\nto 0 for infinite uses.");
        InputShort("Self Effect", &skill->SelfEffect, 1);
        Tooltip("An effect to be applied to the user. Undocumented,\nneeds more research.");
        InputShort("Button Restrictions", &skill->ButtonRestrictions, 1);
        ImGui::TableNextColumn();
        InputShort("Requirement Type", &skill->Requirements, 1);
        Tooltip("The skill's type of special requirement.\n0 = None\n1 = Health\n5 = Skills Left in Deck\n7 = Aura\n9 = Level");
        InputShort("Requirement Amount", &skill->ReqAmount, 1);
        Tooltip("The required amount of the type specified in\nthe previous box");

        ImGui::SetNextItemWidth(200);
        if (ui_state.limitless) {
            InputU32("Skill Use Restrictions", (int*)&skill->GroundAirBoth, 1);
        }
        else {
            const char *items[] = {"Ground", "Air", "Both"};
            ImGui::SliderInt("Skill Use Restrictions", (int *) &skill->GroundAirBoth, 0, 2,items[skill->GroundAirBoth % 2]);
        }
        Tooltip("Where the skill may be used.");

        InputShort("Skill Button Effect", &skill->SkillButtonEffect, 1);
        ImGui::TableNextColumn();
        InputShort("Applied Status ID", &skill->AppliedStatusID, 1);
        Tooltip("An effect to be applied to the target.\n1 = Aura Drain\n2 = Aura Level Decrease\n3 = Aura Drain\n4 = Aura Level Decrease\n5 = Explode\n6 = Paralysis\n7 = Frozen\n8 = Poison\n9 = Death\n10 = Freeze Same Button\n11 = Absorb Aura");
        InputShort("Restrictions", &skill->Restriction, 1);
        ImGui::TableNextColumn();
        InputShort("Strength Effect", &skill->StrengthEffect, 1);
        ImGui::TableNextColumn();
        InputShort("Damage", &skill->Damage, 1);
        ImGui::TableNextColumn();
        InputShort("Effect Duration / Misc. Effects", &skill->EffectDuration, 1);
        ImGui::TableNextColumn();
        InputShort("Target Hand Data", &skill->TargetHand, 1);
        ImGui::TableNextColumn();
        InputShort("Hit Effect Skills", &skill->HitEffectSkills, 1);
        ImGui::TableNextColumn();
        InputShort("Increase Stat", &skill->Increase, 1);
        ImGui::TableNextColumn();
        InputUInt8("Status Enabler", &skill->StatusEnabler);
        ImGui::TableNextColumn();
        InputUInt8("Status ID Duration", &skill->StatusDuration);
        ImGui::TableNextColumn();
        InputShort("Projectile Properties", &skill->ProjectileProperties, 1);
        ImGui::TableNextColumn();
        InputShort("Projectile ID", &skill->ProjectileID, 1);
        ImGui::TableNextColumn();
        InputShort("\"Collision Skill ID\"", &skill->CollisionSkillID, 1);
        ImGui::TableNextColumn();
        InputShort("Homing Range 1st Hit", &skill->HomingRangeFirstHit, 1);
        ImGui::TableNextColumn();
        InputShort("Knockback Strength", &skill->HomingRangeSecondHit, 1);
        ImGui::TableNextColumn();
        InputShort("Combo End", &skill->HomingRangeThirdHit, 1);
        ImGui::TableNextColumn();
        InputShort("Projectile Behaviour", &skill->ProjectileBehaviour, 1);
        ImGui::TableNextColumn();
        if (skill->ProjectileBehaviour > 20) {
            // The game will crash if this is over 0x14
            skill->ProjectileBehaviour = 20;
        }
        InputUInt8("Skill Duration", &skill->SkillDuration);
        ImGui::TableNextColumn();
        InputUInt8("Hitbox Size", &skill->hitbox_size);
        ImGui::TableNextColumn();
        InputShort("Expand Skill Width / Start Speed", &skill->ExpandSkillWidth, 1);
        ImGui::TableNextColumn();
        InputShort("Animation Size / Acceleration", &skill->AnimationSize, 1);
        ImGui::TableNextColumn();
        InputShort("Projectile End Speed", &skill->ProjectileSpeed, 1);
        ImGui::TableNextColumn();
        InputShort("Homing Strength / Accuracy", &skill->AccuracyID, 1);
        ImGui::TableNextColumn();
        InputShort("Animation Height", &skill->AnimationHeight, 1);

        ImGui::EndTable();
    }

    ImGui::PopStyleVar();
    ImGui::End();
}


// Markdown setup stuff

static ImFont* H1 = nullptr;
static ImFont* H2 = nullptr;
static ImFont* H3 = nullptr;

static ImGui::MarkdownConfig mdConfig;
void LinkCallback(ImGui::MarkdownLinkCallbackData data_) {
    std::string url(data_.link, data_.linkLength);
    if (!data_.isImage) {
        ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

inline ImGui::MarkdownImageData ImageCallback(ImGui::MarkdownLinkCallbackData data_) {
    // In your application you would load an image based on data_ input. Here we just use the imgui font texture.
    ImTextureID image = ImGui::GetIO().Fonts->TexID;
    // > C++14 can use ImGui::MarkdownImageData imageData{ true, false, image, ImVec2( 40.0f, 20.0f ) };
    ImGui::MarkdownImageData imageData;
    imageData.isValid = true;
    imageData.useLinkCallback = false;
    imageData.user_texture_id = image;
    imageData.size = ImVec2(40.0f, 20.0f);

    // For image resize when available size.x > image width, add
    ImVec2 const contentSize = ImGui::GetContentRegionAvail();
    if (imageData.size.x > contentSize.x) {
        float const ratio = imageData.size.y / imageData.size.x;
        imageData.size.x = contentSize.x;
        imageData.size.y = contentSize.x * ratio;
    }

    return imageData;
}

void ExampleMarkdownFormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo_, bool start_) {
    // Call the default first so any settings can be overwritten by our implementation.
    // Alternatively could be called or not called in a switch statement on a case by case basis.
    // See defaultMarkdownFormatCallback definition for further examples of how to use it.
    ImGui::defaultMarkdownFormatCallback(markdownFormatInfo_, start_);

    switch (markdownFormatInfo_.type) {
        // example: change the colour of heading level 2
        case ImGui::MarkdownFormatType::HEADING: {
            if (markdownFormatInfo_.level == 2) {
                if (start_) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                }
                else {
                    ImGui::PopStyleColor();
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

void Markdown(const std::string& markdown_) {
    // You can make your own Markdown function with your preferred string container and markdown config.
    // > C++14 can use ImGui::MarkdownConfig mdConfig{ LinkCallback, NULL, ImageCallback, ICON_FA_LINK, { { H1, true }, { H2, true }, { H3, false } }, NULL };
    mdConfig.linkCallback = LinkCallback;
    mdConfig.tooltipCallback = nullptr;
    mdConfig.imageCallback = ImageCallback;
    mdConfig.linkIcon = nullptr; // Was "ICON_FA_LINK" in example
    mdConfig.headingFormats[0] = { H1, true };
    mdConfig.headingFormats[1] = { H2, true };
    mdConfig.headingFormats[2] = { H3, false };
    mdConfig.userData = nullptr;
    mdConfig.formatCallback = ExampleMarkdownFormatCallback;
    ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);
}
