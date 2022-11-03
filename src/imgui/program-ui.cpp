#include <iostream>

#include <imgui.h>
#include <imgui_hex_editor.h>
#include <imgui_markdown.h>

#include "../winAPI.h"
#include "imgui_backend.h"
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
}ui_state;

char packname_internal[32]; // Mod name stored inside the binary file (NOT the filename)
short timer;
bool GamePaused = false;
short ID = 0;
short SelectIdx = 0;

static const char* DocumentationAtkBody[50] = {
#include "../res/AttackSkillBody.txt"
};

static const char* DocumentationAtkLabels[50] = {
#include "../res/AttackSkillLabels.txt"
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

void InputShort(const char* label, void* p_data)
{
    ImGui::SetNextItemWidth(200);
    ImGui::InputScalar(label, ImGuiDataType_S16, p_data, (const void*)1);
}

void InputUInt8(const char* label, void* p_data)
{
    ImGui::SetNextItemWidth(200);
    ImGui::InputScalar(label, ImGuiDataType_S8, p_data, (const void*) 1);
}

int ProgramUI()
{
    ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
    float height = ImGui::GetFrameHeight();

    ImGui::DockSpaceOverViewport(); // Enable docking

    if (ImGui::BeginViewportSideBar("MenuBar", viewport, ImGuiDir_Up, height, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Skill Pack", "Ctrl + N"))
                {
                    if (SUCCEEDED(MultiSelectWindow()))
                    {
                        ui_state.NewSkillPack = true;
                    }
                    else {
                        std::cout << "Skill selection canceled.\n";
                    }
                }
                if (ImGui::BeginMenu("Open"))
                {
                    if (ImGui::MenuItem("Skill (From Memory)"))
                    {
                        if (GetProcess())
                        {
                            ui_state.IDSelection = true;
                        }
                    }
                    if (ImGui::MenuItem("Skill File"))
                    {
                        if (FileSelectDialog(COMDLG_FILTERSPEC{ L"Skill File", L"*.skill;" }) != -1) // Open a file open dialog
                        {
                            LoadAttackSkill();        // Loads the current file into the atkskill struct
                            std::cout << "Imported attack skill " << filepath << "\n";
                            ui_state.AttackSkillEditor = true;    // Opens the Attack Skill Editor window
                        }
                        else
                        {
                            std::cout << "File selection canceled.\n";
                        }
                    }
                    if (ImGui::MenuItem("Install Skill Pack"))
                    {
                        GetProcess();
                        if (SUCCEEDED(MultiSelectWindow())) // Open a multiple file open dialog
                        {
                            InstallSkillPackToRAM();
                            for (int i = 0; i < MultiSelectCount; i++)
                            {
                                std::cout << "Installed skill pack " << multiselectpath[i] << ".\n";
                            }
                        }
                        else
                        {
                            std::cout << "File selection canceled.\n";
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Save", "Ctrl + S"))
                {
                    SaveAtkSkill();
                }
                if (ImGui::MenuItem("Save As", "Ctrl + Shift + S"))
                {
                    if (FileSaveDialog(COMDLG_FILTERSPEC{ L"Skill Pack", L"*.bin;" }, L".skill") != -1) // Open a file save dialog and save to a new file
                    {
                        SaveAtkSkill(); // Write data.
                    }
                    else
                    {
                        std::cout << "File selection canceled.\n";
                    }
                }

                if (ImGui::MenuItem("Exit", "Alt + F4"))
                {
                    return 1;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window"))
            {
                if (ImGui::MenuItem("Attack Skill Editor"))
                {
                    ui_state.AttackSkillEditor = !ui_state.AttackSkillEditor; // Toggle Attack Skill Editor window
                }
                if (ImGui::MenuItem("Skill Hex Editor"))
                {
                    GetProcess();
                    if (LoadGSDataFromRAM() == 0)
                    {
                        ui_state.HexEditor = !ui_state.HexEditor;
                    }
                }
                if (ImGui::MenuItem("Documentation"))
                {
                    ui_state.Documentation = !ui_state.Documentation;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Game"))
            {
                if (ImGui::MenuItem("Freeze/Unfreeze Phantom Dust"))
                {
                    if (GamePaused) {
                        UnpauseGame();
                        GamePaused = true;
                    }
                    else {
                        PauseGame();
                    }
                    GamePaused = !GamePaused;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
    }

    if (timer == 0)
    {
        // Save: Ctrl + S
        if (GetKeyState(VK_CONTROL) & GetKeyState('S') & 0x8000)
        {
            timer = 20;

            // Save As: Ctrl + Shift + S
            if (GetKeyState(VK_SHIFT))
            {
                if (FileSaveDialog(COMDLG_FILTERSPEC{ L"Skill Pack", L"*.bin;" }, L".skill") != -1)
                {
                    SaveAtkSkill(); // Write data.
                }
                else
                {
                    std::cout << "File selection cancelled.\n";
                }
            }
            else
            {
                SaveAtkSkill(); // Write data.
            }
        }

        // New: Ctrl + N
        if (GetKeyState(VK_CONTROL) & GetKeyState('N') & 0x8000 && !ui_state.NewSkillPack)
        {
            if (SUCCEEDED(MultiSelectWindow()))
            {
                ui_state.NewSkillPack = true;
            }
            else {
                std::cout << "Skill selection canceled.\n";
            }
            timer = 20;
        }
    }
    else
    {
        timer--; // Decrement cooldown timer until it hits 0
    }

    if (ui_state.IDSelection)
    {
        ImGui::Begin("Input a skill ID: ");
        InputShort("ID", &ID);

        if (ImGui::Button("Open"))
        {
            memcpy(&AtkSkill, &skillarray[ID], 144);
            std::cout << "Loaded skill with ID " << ID << ".\n";
            ui_state.IDSelection = false;      // Close this window
            ui_state.AttackSkillEditor = true; // Opens the Attack Skill Editor window
        }

        ImGui::End();
    }

    if (ui_state.HexEditor)
    {
        hex_edit.ReadOnly = false;
        hex_edit.DrawWindow("Hex Editor", &skillarray[ID], 144);
    }

    if (ui_state.AttackSkillEditor)
    {
        // This is the only window that's not inlined, because it would actually make the flow of logic harder to follow.
        AtkSkillWindow();
    }

    if (ui_state.Documentation)
    {
        ImGui::SetNextWindowSize(ImVec2(850, 650), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Documentation", &ui_state.Documentation))
        {
            ImGui::End();
        }

        if (ImGui::BeginTabBar("DocTabs"))
        {
            if (ImGui::BeginTabItem("Attack Skills"))
            {
                ImGui::BeginChild("left pane", ImVec2(200, 0), true);
                for (short i = 0; i < IM_ARRAYSIZE(DocumentationAtkLabels); i++)
                {
                    // Selectable object for every string in the array
                    if (ImGui::Selectable(DocumentationAtkLabels[i]))
                    {
                        SelectIdx = i;
                    }
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("Doc Text", ImVec2(600, 0), false);
                // Renders the item name as an H1, then the description on a new line.
                std::string md = "# ";
                md += DocumentationAtkLabels[SelectIdx];
                md += DocumentationAtkBody[SelectIdx];
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
        ImGui::Begin("Enter a name for your skill pack: ");
        ImGui::InputText("Skill Pack Name", packname_internal, 32);

        if (ImGui::Button("Save"))
        {
            if (SUCCEEDED(FileSaveDialog(COMDLG_FILTERSPEC{ L"Skill Pack", L"*.bin;" }, L".bin")))
            {
                SaveSkillPack(packname_internal);
                ui_state.NewSkillPack = false;
            }
        }

        ImGui::End();
    }
    return 0;
}

void AtkSkillWindow()
{
    std::string WindowTitle = "Attack Skill Editor";
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
        InputShort("Skill Text ID", &AtkSkill.SkillTextID);
        Tooltip("The skill ID from which to get the skill's name,\ndescription, etc.");
        InputShort("Register ID", &AtkSkill.RegisterID);
        Tooltip("The \"register\" / \"library\" of skills this skill\nwill belong to.");
        InputShort("Skill ID", &AtkSkill.SkillID);
        Tooltip("The skill's internal ID. This will determine what\nskill will be overwritten. This internal ID has no\nrelation to the IDs seen in-game.");

        ImGui::SetNextItemWidth(200);
        // Int pointer with value (rarity + 1). This means that the slider function will
        // automatically update the actual rarity value despite it being a short.
        int rarity = (int) (AtkSkill.RarityStars + 1);
        ImGui::SliderInt("Rarity", &rarity, 1, 5);
        AtkSkill.RarityStars = (short) rarity - 1;
        Tooltip("The skill's in-game rarity, displayed as stars.");

        InputShort("Sound File ID", &AtkSkill.SoundFileID);
        ImGui::TableNextColumn();

        ImGui::SetNextItemWidth(200);
        const char* elems_names[7] = { "Aura", "Attack", "Defense", "Erase", "Environmental", "Status", "Special" };
        ImGui::SliderInt("Capsule Type", (int*)&AtkSkill.CapsuleType, 0, 7 - 1, elems_names[AtkSkill.CapsuleType]);
        ImGui::TableNextColumn();

        InputShort("School ID", &AtkSkill.SchoolID);
        Tooltip("The skill's school. (Nature, Optical, Ki, etc.)");
        InputShort("Animation Profile (Ground)", &AtkSkill.AnimationProfileGround);
        Tooltip("This controls the player's skeletal animation, the number\nof projectiles fired, particle effects used, and much more.\nThis profile is used when the skill is cast on the ground.");
        InputShort("Animation Profile (Air)", &AtkSkill.AnimationProfileAir);
        Tooltip("This controls the player's skeletal animation, the number\nof projectiles fired, particle effects used, and much more.\nThis profile is used when the skill is cast in the air.");
        InputShort("Multi Press 1", &AtkSkill.MultiPress1);
        ImGui::TableNextColumn();
        InputShort("Multi Press 2", &AtkSkill.MultiPress2);
        ImGui::TableNextColumn();
        InputShort("Double Skill 1", &AtkSkill.DoubleSkill1);
        ImGui::TableNextColumn();
        InputShort("Double Skill 2", &AtkSkill.DoubleSkill2);
        ImGui::TableNextColumn();
        InputShort("After Hit SFX", &AtkSkill.PostHitSFX);
        ImGui::TableNextColumn();
        InputShort("Start Up SFX", &AtkSkill.StartUpSFX);
        Tooltip("The sound effect ID to be played when the skill\nis winding up / charging.");
        InputShort("Collision SFX", &AtkSkill.CollisionSFX);
        Tooltip("The sound effect ID to be played when the skill\ncollides with something.");
        InputShort("Aura Cost", &AtkSkill.Cost);
        Tooltip("The amount of Aura the skill costs.");
        InputShort("Cost Effect", &AtkSkill.CostEffect);
        Tooltip("Additional special costs.\n0 = None\n1 = Reset Aura\n2 = Require Max Aura");
        InputShort("Additional Aura Cost", &AtkSkill.ExtraCost);
        Tooltip("Adds to the base Aura cost.");
        InputShort("Health Penalty", &AtkSkill.HealthCost);
        Tooltip("The amount of health to be taken from the user\nwhen the skill is cast.");
        InputShort("# of Uses", &AtkSkill.SkillUses);
        Tooltip("How many times the skill can be used. Set this\nto 0 for infinite uses.");
        InputShort("Self Effect", &AtkSkill.SelfEffect);
        Tooltip("An effect to be applied to the user. Undocumented,\nneeds more research.");
        InputShort("Button Restrictions", &AtkSkill.ButtonRestrictions);
        ImGui::TableNextColumn();
        InputShort("Requirement Type", &AtkSkill.Requirements);
        Tooltip("The skill's type of special requirement.\n0 = None\n1 = Health\n5 = Skills Left in Deck\n7 = Aura\n9 = Level");
        InputShort("Requirement Amount", &AtkSkill.ReqAmount);
        Tooltip("The required amount of the type specified in\nthe previous box");

        ImGui::SetNextItemWidth(200);
        const char* items[] = { "Ground","Air","Both" };
        ImGui::SliderInt("Skill Use Restrictions", (int*)&AtkSkill.GroundAirBoth, 0, 2, items[AtkSkill.GroundAirBoth]);
        Tooltip("Where the skill may be used.");

        InputShort("Skill Button Effect", &AtkSkill.SkillButtonEffect);
        ImGui::TableNextColumn();
        InputShort("Applied Status ID", &AtkSkill.AppliedStatusID);
        Tooltip("An effect to be applied to the target.\n1 = Aura Drain\n2 = Aura Level Decrease\n3 = Aura Drain\n4 = Aura Level Decrease\n5 = Explode\n6 = Paralysis\n7 = Frozen\n8 = Poision\n9 = Death\n10 = Freeze Same Button\n11 = Absorb Aura");
        InputShort("Restrictions", &AtkSkill.Restriction);
        ImGui::TableNextColumn();
        InputShort("Strength Effect", &AtkSkill.StrengthEffect);
        ImGui::TableNextColumn();
        InputShort("Damage", &AtkSkill.Damage);
        ImGui::TableNextColumn();
        InputShort("Effect Duration / Misc. Effects", &AtkSkill.EffectDuration);
        ImGui::TableNextColumn();
        InputShort("Target Hand Data", &AtkSkill.TargetHand);
        ImGui::TableNextColumn();
        InputShort("Hit Effect Skills", &AtkSkill.HitEffectSkills);
        ImGui::TableNextColumn();
        InputShort("Increase Stat", &AtkSkill.Increase);
        ImGui::TableNextColumn();
        InputUInt8("Status Enabler", &AtkSkill.StatusEnabler);
        ImGui::TableNextColumn();
        InputUInt8("Status ID Duration", &AtkSkill.StatusDuration);
        ImGui::TableNextColumn();
        InputShort("Projectile Properties", &AtkSkill.ProjectileProperties);
        ImGui::TableNextColumn();
        InputShort("Projectile ID", &AtkSkill.ProjectileID);
        ImGui::TableNextColumn();
        InputShort("\"Collision Skill ID\"", &AtkSkill.ProjectileID);
        ImGui::TableNextColumn();
        InputShort("Homing Range 1st Hit", &AtkSkill.HomingRangeFirstHit);
        ImGui::TableNextColumn();
        InputShort("Knockback Strength", &AtkSkill.HomingRangeSecondHit);
        ImGui::TableNextColumn();
        InputShort("Combo End", &AtkSkill.HomingRangeThirdHit);
        ImGui::TableNextColumn();
        InputShort("Projectile Behaviour", &AtkSkill.ProjectileBehaviour);
        ImGui::TableNextColumn();
        if (AtkSkill.ProjectileBehaviour > 20)
        {
            // The game will crash if this is over 0x14
            AtkSkill.ProjectileBehaviour = 20;
        }
        InputUInt8("Skill Duration", &AtkSkill.SkillDuration);
        ImGui::TableNextColumn();
        InputUInt8("Hit Range", &AtkSkill.HitRange);
        ImGui::TableNextColumn();
        InputShort("Expand Skill Width / Start Speed", &AtkSkill.ExpandSkillWidth);
        ImGui::TableNextColumn();
        InputShort("Animation Size / Acceleration", &AtkSkill.AnimationSize);
        ImGui::TableNextColumn();
        InputShort("Projectile End Speed", &AtkSkill.ProjectileSpeed);
        ImGui::TableNextColumn();
        InputShort("Homing Strength / Accuracy", &AtkSkill.AccuracyID);
        ImGui::TableNextColumn();
        InputShort("Animation Height", &AtkSkill.AnimationHeight);

        ImGui::EndTable();
    }

    ImGui::PopStyleVar();
    ImGui::End();
}


// Markdown setup stuff

static ImFont* H1 = NULL;
static ImFont* H2 = NULL;
static ImFont* H3 = NULL;

static ImGui::MarkdownConfig mdConfig;
void LinkCallback(ImGui::MarkdownLinkCallbackData data_)
{
    std::string url(data_.link, data_.linkLength);
    if (!data_.isImage)
    {
        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
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

void LoadFonts(float fontSize_)
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    // Base font
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", fontSize_);
    // Bold headings H2 and H3
    H2 = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", fontSize_); // Bold font, I don't have one so I'm just using the same font
    H3 = mdConfig.headingFormats[1].font;
    // bold heading H1
    float fontSizeH1 = fontSize_ * 1.1f;
    H1 = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", fontSizeH1); // Also supposed to be bold
}

void ExampleMarkdownFormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo_, bool start_)
{
    // Call the default first so any settings can be overwritten by our implementation.
    // Alternatively could be called or not called in a switch statement on a case by case basis.
    // See defaultMarkdownFormatCallback definition for furhter examples of how to use it.
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
    // You can make your own Markdown function with your prefered string container and markdown config.
    // > C++14 can use ImGui::MarkdownConfig mdConfig{ LinkCallback, NULL, ImageCallback, ICON_FA_LINK, { { H1, true }, { H2, true }, { H3, false } }, NULL };
    mdConfig.linkCallback = LinkCallback;
    mdConfig.tooltipCallback = NULL;
    mdConfig.imageCallback = ImageCallback;
    mdConfig.linkIcon = NULL; // Was "ICON_FA_LINK" in example
    mdConfig.headingFormats[0] = { H1, true };
    mdConfig.headingFormats[1] = { H2, true };
    mdConfig.headingFormats[2] = { H3, false };
    mdConfig.userData = NULL;
    mdConfig.formatCallback = ExampleMarkdownFormatCallback;
    ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);
}
