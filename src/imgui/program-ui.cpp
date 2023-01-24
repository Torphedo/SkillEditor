#include <iostream>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_hex_editor.h>
#include <imgui_markdown.h>

#include "../winAPI.h"
#include "program-ui.h"
#include "../skill_io.h"
#include "../memory-editing.h"

struct
{
    bool NewSkillPack;
    bool HexEditor;
    bool AttackSkillEditor;
    bool Documentation;
    bool IDSelection;
    bool text_edit;

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

static const char* DocumentationProgramBody[3] = {
#include "../res/SkillEditorBody.txt"
};

static const char* DocumentationProgramLabels[3] = {
#include "../res/SkillEditorLabels.txt"
};

void AtkSkillWindow();
static void Markdown(const std::string& markdown_); // Markdown function prototype
static MemoryEditor hex_edit;

void Tooltip(const char* text)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", text);
    }
    ImGui::TableNextColumn();
}

// ImGui didn't have pre-made functions for short
// or uint8 input boxes, so I made my own.

void InputShort(const char* label, void* p_data, unsigned short step)
{
    ImGui::SetNextItemWidth(200);
    ImGui::InputScalar(label, ImGuiDataType_U16, p_data, &step);
}

void InputUInt8(const char* label, void* p_data)
{
    static constexpr int step = 1;
    ImGui::SetNextItemWidth(200);
    ImGui::InputScalar(label, ImGuiDataType_U8, p_data, &step);
}

int ProgramUI()
{
    auto viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
    float height = ImGui::GetFrameHeight();

    ImGui::DockSpaceOverViewport(); // Enable docking

    if (ImGui::BeginViewportSideBar("MenuBar", viewport, ImGuiDir_Up, height, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Skill Pack", "N"))
                {
                    if (SUCCEEDED(file_multiple_select_dialog()))
                    {
                        ui_state.NewSkillPack = true;
                    }
                    else {
                        printf("Skill selection canceled.\n");
                    }
                }
                if (ImGui::BeginMenu("Open"))
                {
                    if (ImGui::MenuItem("Skill (From Memory)"))
                    {
                        if (get_process())
                        {
                            ui_state.IDSelection = true;
                        }
                    }
                    if (ImGui::MenuItem("Skill File"))
                    {
                        ID = load_attack_skill(ID);
                        write_gsdata_to_memory();
                        ui_state.AttackSkillEditor = true; // Open the Attack Skill Editor window
                    }
                    if (ImGui::MenuItem("Install Skill Pack"))
                    {
                        if (get_process())
                        {
                            if (SUCCEEDED(file_multiple_select_dialog())) // Open a multiple file open dialog
                            {
                                install_mod();
                                for (int i = 0; i < MultiSelectCount; i++)
                                {
                                    std::cout << "Installed skill pack " << multiselectpath[i] << ".\n";
                                }
                            }
                            else
                            {
                                printf("File selection canceled.\n");
                            }
                        }
                        else {
                            printf("Can't access game's skill data in memory, cancelling skill pack install.\n");
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Save To File", "S"))
                {
                    save_skill_to_file(ID);
                }
                if (ImGui::MenuItem("Save To Memory"))
                {
                    write_gsdata_to_memory();
                }
                if (ImGui::MenuItem("Save As", "Ctrl + S"))
                {
                    save_skill_with_dialog(ID);
                }

                if (ImGui::MenuItem("Exit", "Alt + F4"))
                {
                    return 1;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window"))
            {
                ImGui::MenuItem("Attack Skill Editor", nullptr, &ui_state.AttackSkillEditor);
                if (ImGui::MenuItem("Skill Hex Editor"))
                {
                    if (get_process())
                    {
                        ui_state.HexEditor = !ui_state.HexEditor;
                    }
                }
                ImGui::MenuItem("Documentation", nullptr, &ui_state.Documentation);
                if (ImGui::MenuItem("Text Edit", nullptr, &ui_state.text_edit))
                {
                    get_process(); // Refresh skill data address & game handle

                    skill_text text = load_skill_text(ID);
                    ui_state.current_name = text.name;
                    ui_state.current_desc = text.desc;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Game")) {
                if (ImGui::MenuItem("Freeze/Unfreeze Phantom Dust"))
                {
                    toggle_game_pause();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings")) {
                static bool enabled = true;
                ImGui::Checkbox("File Mode", &enabled);
                ImGui::EndMenu();
            }
            
            if (!is_running())
            {
                // Alignment to right side
                ImGui::Text("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\
                            \t\t\t\t\t\t\t");
                ImGui::TextColored({ 255, 0, 0, 255 }, "No Phantom Dust instance detected!");
            }
            else
            {
                // Alignment to right side
                ImGui::Text("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
                if (!have_process_handle()) 
                {
                    ImGui::TextColored({ 255, 0, 0, 255 }, "No handle to process!");
                }
                else if (!can_read_memory())
                {
                    ImGui::TextColored({ 255, 0, 0, 255 }, "Can't read from process!");
                }
                else { ImGui::Text("\t\t\t\t\t\t\t\t\t\t"); }

                if (ImGui::Button("Retry connection")) { printf("Retrying..."); get_process(); }

                ImGui::Text("Phantom Dust Process ID:");
                ImGui::TextColored({ 0, 255, 0, 255 }, "%li", process_id());
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
    }

    // ImGui::ShowDemoWindow();

    if (ImGui::IsKeyPressed(ImGuiKey_F4)) {
        toggle_game_pause();
    }

    // Save
    if (ImGui::IsKeyPressed(ImGuiKey_S)) {
        // Save As
        if (ImGui::IsKeyPressed(ImGuiKey_LeftCtrl) || ImGui::IsKeyPressed(ImGuiKey_RightCtrl)) {
            save_skill_with_dialog(ID);
        } else {
            save_skill_to_file(ID);
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_N, false) && !ui_state.NewSkillPack)
    {
        if (SUCCEEDED(file_multiple_select_dialog())) {
            ui_state.NewSkillPack = true;
        }
    }

    if (ui_state.IDSelection)
    {
        // Temporary storage to hold an ID before actually updating the selected ID
        static uint16_t temp_id = 0;
        ImGui::Begin("Input a skill ID: ");
        InputShort("ID", &temp_id, 1);
        if (ImGui::Button("Open"))
        {
            ID = temp_id;
            printf("Loaded skill with ID %d\n", ID);
            ui_state.IDSelection = false;      // Close this window
            ui_state.AttackSkillEditor = true; // Opens the Attack Skill Editor window
        }
        ImGui::End();
    }

    if (ui_state.HexEditor)
    {
        hex_edit.ReadOnly = false;
        hex_edit.DrawWindow("Hex Editor", &gstorage.skill_array[ID - 1], 144);
    }

    if (ui_state.AttackSkillEditor)
    {
        // This is the only window that's not inlined, because it would actually make the flow of logic harder to follow.
        AtkSkillWindow();
    }

    if (ui_state.Documentation)
    {
        ImGui::SetNextWindowSize(ImVec2(850, 650), ImGuiCond_FirstUseEver);
        ImGui::Begin("Documentation", &ui_state.Documentation);

        if (ImGui::BeginTabBar("DocTabs"))
        {
            if (ImGui::BeginTabItem("Attack Skills"))
            {
                static uint16_t select_idx = 0;
                ImGui::BeginChild("left pane", ImVec2(200, 0), true);
                for (unsigned short i = 0; i < (unsigned short)IM_ARRAYSIZE(DocumentationAtkLabels); i++)
                {
                    // Selectable object for every string in the array
                    if (ImGui::Selectable(DocumentationAtkLabels[i]))
                    {
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

            if (ImGui::BeginTabItem("Skill Editor"))
            {
                static uint16_t select_idx = 0;
                ImGui::BeginChild("left pane", ImVec2(200, 0), true);
                for (unsigned short i = 0; i < (unsigned short)IM_ARRAYSIZE(DocumentationProgramLabels); i++)
                {
                    // Selectable object for every string in the array
                    if (ImGui::Selectable(DocumentationProgramLabels[i]))
                    {
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


    if (ui_state.NewSkillPack)
    {
        static char packname_internal[32] = {0}; // Mod name that will be stored in the binary
        ImGui::Begin("Enter a name for your skill pack: ");
        ImGui::InputText("Skill Pack Name", packname_internal, 32);
        if (ImGui::Button("Save"))
        {
            save_skill_pack(packname_internal);
            ui_state.NewSkillPack = false;
        }
        ImGui::End();
    }

    if (ui_state.text_edit)
    {
        ImGui::Begin("Skill Text Editor", &ui_state.text_edit);
        // Allow text input, limited to the size of the original text
        ImGui::InputText("Skill Name", ui_state.current_name.data(), ui_state.current_name.length() + 1);
        ImGui::InputText("Skill Description", ui_state.current_desc.data(), ui_state.current_desc.length() + 1);

        static uint16_t text_id = 0;
        uint16_t cache = text_id; // Previously selected skill ID
        text_id = gstorage.skill_array[ID - 1].SkillTextID + 1;

        if (ImGui::Button("Reload") || cache != text_id)
        {
            skill_text text = load_skill_text(text_id);
            ui_state.current_name = text.name;
            ui_state.current_desc = text.desc;
        }

        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            skill_text text = {ui_state.current_name, ui_state.current_desc};
            save_skill_text(text, text_id);
        }
        ImGui::End();
    }
    return 0;
}

void AtkSkillWindow()
{
    if (!ImGui::Begin("Attack Skill Editor", &ui_state.AttackSkillEditor))
    {
        ImGui::End();
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    ImGui::AlignTextToFramePadding();

    if (ImGui::BeginTable("split1", 2, ImGuiTableFlags_NoSavedSettings))
    {
        ImGui::TableNextColumn();
        InputShort("Skill Text ID", &gstorage.skill_array[ID - 1].SkillTextID, 1);
        Tooltip("The skill ID from which to get the skill's name,\ndescription, etc.");
        InputShort("Register ID", &gstorage.skill_array[ID - 1].RegisterID, 1);
        Tooltip("The \"register\" / \"library\" of skills this skill\nwill belong to.");
        InputShort("Skill ID", &gstorage.skill_array[ID - 1].SkillID, 1);
        Tooltip("The skill's internal ID. This will determine what\nskill will be overwritten. This internal ID has no\nrelation to the IDs seen in-game.");

        ImGui::SetNextItemWidth(200);
        // Int pointer with value (rarity + 1). This means that the slider function will
        // automatically update the actual rarity value despite it being a short.
        int rarity = (int) (gstorage.skill_array[ID - 1].RarityStars + 1);
        ImGui::SliderInt("Rarity", &rarity, 1, 5);
        gstorage.skill_array[ID - 1].RarityStars = (short) rarity - 1;
        Tooltip("The skill's in-game rarity, displayed as stars.");

        InputShort("Sound File ID", &gstorage.skill_array[ID - 1].SoundFileID, 1);
        ImGui::TableNextColumn();

        ImGui::SetNextItemWidth(200);
        const char* elems_names[7] = { "Aura", "Attack", "Defense", "Erase", "Environmental", "Status", "Special" };
        ImGui::SliderInt("Capsule Type", (int*)&gstorage.skill_array[ID - 1].CapsuleType, 0, 7 - 1, elems_names[gstorage.skill_array[ID - 1].CapsuleType]);
        ImGui::TableNextColumn();

        InputShort("School ID", &gstorage.skill_array[ID - 1].SchoolID, 1);
        Tooltip("The skill's school. (Nature, Optical, Ki, etc.)");
        InputShort("Animation Profile (Ground)", &gstorage.skill_array[ID - 1].AnimationProfileGround, 1);
        Tooltip("This controls the player's skeletal animation, the number\nof projectiles fired, particle effects used, and much more.\nThis profile is used when the skill is cast on the ground.");
        InputShort("Animation Profile (Air)", &gstorage.skill_array[ID - 1].AnimationProfileAir, 1);
        Tooltip("This controls the player's skeletal animation, the number\nof projectiles fired, particle effects used, and much more.\nThis profile is used when the skill is cast in the air.");
        InputShort("Multi Press 1", &gstorage.skill_array[ID - 1].MultiPress1, 1);
        ImGui::TableNextColumn();
        InputShort("Multi Press 2", &gstorage.skill_array[ID - 1].MultiPress2, 1);
        ImGui::TableNextColumn();
        InputShort("Double Skill 1", &gstorage.skill_array[ID - 1].DoubleSkill1, 1);
        ImGui::TableNextColumn();
        InputShort("Double Skill 2", &gstorage.skill_array[ID - 1].DoubleSkill2, 1);
        ImGui::TableNextColumn();
        InputShort("After Hit SFX", &gstorage.skill_array[ID - 1].PostHitSFX, 1);
        ImGui::TableNextColumn();
        InputShort("Start Up SFX", &gstorage.skill_array[ID - 1].StartUpSFX, 1);
        Tooltip("The sound effect ID to be played when the skill\nis winding up / charging.");
        InputShort("Collision SFX", &gstorage.skill_array[ID - 1].CollisionSFX, 1);
        Tooltip("The sound effect ID to be played when the skill\ncollides with something.");
        InputShort("Aura Cost", &gstorage.skill_array[ID - 1].Cost, 1);
        Tooltip("The amount of Aura the skill costs.");
        InputShort("Cost Effect", &gstorage.skill_array[ID - 1].CostEffect, 1);
        Tooltip("Additional special costs.\n0 = None\n1 = Reset Aura\n2 = Require Max Aura");
        InputShort("Additional Aura Cost", &gstorage.skill_array[ID - 1].ExtraCost, 1);
        Tooltip("Adds to the base Aura cost.");
        InputShort("Health Penalty", &gstorage.skill_array[ID - 1].HealthCost, 1);
        Tooltip("The amount of health to be taken from the user\nwhen the skill is cast.");
        InputShort("# of Uses", &gstorage.skill_array[ID - 1].SkillUses, 1);
        Tooltip("How many times the skill can be used. Set this\nto 0 for infinite uses.");
        InputShort("Self Effect", &gstorage.skill_array[ID - 1].SelfEffect, 1);
        Tooltip("An effect to be applied to the user. Undocumented,\nneeds more research.");
        InputShort("Button Restrictions", &gstorage.skill_array[ID - 1].ButtonRestrictions, 1);
        ImGui::TableNextColumn();
        InputShort("Requirement Type", &gstorage.skill_array[ID - 1].Requirements, 1);
        Tooltip("The skill's type of special requirement.\n0 = None\n1 = Health\n5 = Skills Left in Deck\n7 = Aura\n9 = Level");
        InputShort("Requirement Amount", &gstorage.skill_array[ID - 1].ReqAmount, 1);
        Tooltip("The required amount of the type specified in\nthe previous box");

        ImGui::SetNextItemWidth(200);
        const char* items[] = { "Ground","Air","Both" };
        ImGui::SliderInt("Skill Use Restrictions", (int*)&gstorage.skill_array[ID - 1].GroundAirBoth, 0, 2, items[gstorage.skill_array[ID - 1].GroundAirBoth]);
        Tooltip("Where the skill may be used.");

        InputShort("Skill Button Effect", &gstorage.skill_array[ID - 1].SkillButtonEffect, 1);
        ImGui::TableNextColumn();
        InputShort("Applied Status ID", &gstorage.skill_array[ID - 1].AppliedStatusID, 1);
        Tooltip("An effect to be applied to the target.\n1 = Aura Drain\n2 = Aura Level Decrease\n3 = Aura Drain\n4 = Aura Level Decrease\n5 = Explode\n6 = Paralysis\n7 = Frozen\n8 = Poison\n9 = Death\n10 = Freeze Same Button\n11 = Absorb Aura");
        InputShort("Restrictions", &gstorage.skill_array[ID - 1].Restriction, 1);
        ImGui::TableNextColumn();
        InputShort("Strength Effect", &gstorage.skill_array[ID - 1].StrengthEffect, 1);
        ImGui::TableNextColumn();
        InputShort("Damage", &gstorage.skill_array[ID - 1].Damage, 1);
        ImGui::TableNextColumn();
        InputShort("Effect Duration / Misc. Effects", &gstorage.skill_array[ID - 1].EffectDuration, 1);
        ImGui::TableNextColumn();
        InputShort("Target Hand Data", &gstorage.skill_array[ID - 1].TargetHand, 1);
        ImGui::TableNextColumn();
        InputShort("Hit Effect Skills", &gstorage.skill_array[ID - 1].HitEffectSkills, 1);
        ImGui::TableNextColumn();
        InputShort("Increase Stat", &gstorage.skill_array[ID - 1].Increase, 1);
        ImGui::TableNextColumn();
        InputUInt8("Status Enabler", &gstorage.skill_array[ID - 1].StatusEnabler);
        ImGui::TableNextColumn();
        InputUInt8("Status ID Duration", &gstorage.skill_array[ID - 1].StatusDuration);
        ImGui::TableNextColumn();
        InputShort("Projectile Properties", &gstorage.skill_array[ID - 1].ProjectileProperties, 1);
        ImGui::TableNextColumn();
        InputShort("Projectile ID", &gstorage.skill_array[ID - 1].ProjectileID, 1);
        ImGui::TableNextColumn();
        InputShort("\"Collision Skill ID\"", &gstorage.skill_array[ID - 1].ProjectileID, 1);
        ImGui::TableNextColumn();
        InputShort("Homing Range 1st Hit", &gstorage.skill_array[ID - 1].HomingRangeFirstHit, 1);
        ImGui::TableNextColumn();
        InputShort("Knockback Strength", &gstorage.skill_array[ID - 1].HomingRangeSecondHit, 1);
        ImGui::TableNextColumn();
        InputShort("Combo End", &gstorage.skill_array[ID - 1].HomingRangeThirdHit, 1);
        ImGui::TableNextColumn();
        InputShort("Projectile Behaviour", &gstorage.skill_array[ID - 1].ProjectileBehaviour, 1);
        ImGui::TableNextColumn();
        if (gstorage.skill_array[ID - 1].ProjectileBehaviour > 20)
        {
            // The game will crash if this is over 0x14
            gstorage.skill_array[ID - 1].ProjectileBehaviour = 20;
        }
        InputUInt8("Skill Duration", &gstorage.skill_array[ID - 1].SkillDuration);
        ImGui::TableNextColumn();
        InputUInt8("Hit Range", &gstorage.skill_array[ID - 1].HitRange);
        ImGui::TableNextColumn();
        InputShort("Expand Skill Width / Start Speed", &gstorage.skill_array[ID - 1].ExpandSkillWidth, 1);
        ImGui::TableNextColumn();
        InputShort("Animation Size / Acceleration", &gstorage.skill_array[ID - 1].AnimationSize, 1);
        ImGui::TableNextColumn();
        InputShort("Projectile End Speed", &gstorage.skill_array[ID - 1].ProjectileSpeed, 1);
        ImGui::TableNextColumn();
        InputShort("Homing Strength / Accuracy", &gstorage.skill_array[ID - 1].AccuracyID, 1);
        ImGui::TableNextColumn();
        InputShort("Animation Height", &gstorage.skill_array[ID - 1].AnimationHeight, 1);

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
void LinkCallback(ImGui::MarkdownLinkCallbackData data_)
{
    std::string url(data_.link, data_.linkLength);
    if (!data_.isImage)
    {
        ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

inline ImGui::MarkdownImageData ImageCallback(ImGui::MarkdownLinkCallbackData data_)
{
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
    if (imageData.size.x > contentSize.x)
    {
        float const ratio = imageData.size.y / imageData.size.x;
        imageData.size.x = contentSize.x;
        imageData.size.y = contentSize.x * ratio;
    }

    return imageData;
}

void ExampleMarkdownFormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo_, bool start_)
{
    // Call the default first so any settings can be overwritten by our implementation.
    // Alternatively could be called or not called in a switch statement on a case by case basis.
    // See defaultMarkdownFormatCallback definition for further examples of how to use it.
    ImGui::defaultMarkdownFormatCallback(markdownFormatInfo_, start_);

    switch (markdownFormatInfo_.type)
    {
        // example: change the colour of heading level 2
    case ImGui::MarkdownFormatType::HEADING:
    {
        if (markdownFormatInfo_.level == 2)
        {
            if (start_)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            }
            else
            {
                ImGui::PopStyleColor();
            }
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

void Markdown(const std::string& markdown_)
{
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
