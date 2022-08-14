// Editor windows, custom functions to be triggered by the UI, and other miscellaneous
// code that doesn't need to be in the main UI window file (UI.cpp)

#include <main.h>
#include <editors.h>
#include <hex_editor/imgui_hex_editor.h>

using std::cout;

short ErrorCode;
bool OptionsWindow = false;
bool RenderSkillPackWindow = false;
short AtkSkillState = 0; // 0 = None, 1 = Opened, 2 = Saved
bool RenderAtkSkillWindow = false;
bool RenderDocumentationWindow = false;

static MemoryEditor hex_edit;

void SafeAtkSave()
{
    if (AtkSkillState != 0) {
        SaveAtkSkill();    // Write data.
        cout << "Saved attack skill to " << filepath << "\n";
        AtkSkillState = 2; // Causes a message to appear on the status bar
    }
    else {
        cout << "Tried to save without opening a file, aborting...\n";
        ErrorCode = 2;
    }
}

void SafeAtkSaveAs()
{
    if (AtkSkillState != 0) {
        if (FileSaveDialog(skillfile, L".skill") != -1) // Open a file save dialog and save to a new file
        {
            SaveAtkSkill();         // Write data.
            cout << "Saved attack skill to " << filepath << "\n";
            AtkSkillState = 2;
        }
        else
        {
            cout << "File selection canceled.\n";
            ErrorCode = 1;
        }
    }
    else {
        cout << "Tried to save without opening a file, aborting...\n";
        ErrorCode = 2;
    }
}

void SafeNewPack()
{
    if (SUCCEEDED(MultiSelectWindow()))
    {
        RenderSkillPackWindow = true;
    }
    else {
        cout << "Skill selection canceled.\n";
        ErrorCode = 2;
    }
}

void DocumentationWindow()
{
    ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Documentation");
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("DocTabs", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Attack Skills"))
        {
            ImGui::Text("Attack Skill Documentation\nblah blah blah blah blah");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void AtkSkillWindow()
{
    std::string WindowTitle = "Attack Skill Editor - " + filepath; // Use filename in the window title.
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(const_cast<char*>(WindowTitle.c_str())))
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
        int* rarity = (int*) (&AtkSkill.RarityStars + 1);
        ImGui::SliderInt("Rarity", rarity, 1, 5);
        Tooltip("The skill's in-game rarity, displayed as stars.");

        InputShort("Sound File ID", &AtkSkill.SoundFileID);
        ImGui::TableNextColumn();

        ImGui::SetNextItemWidth(200);
        const char* elems_names[7] = { "Aura", "Attack", "Defense", "Erase", "Environmental", "Status", "Special" };
        ImGui::SliderInt("Capsule Type", (int*) &AtkSkill.CapsuleType, 0, 7 - 1, elems_names[AtkSkill.CapsuleType]);
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
        char* items[] = { "Ground","Air","Both" };
        ImGui::SliderInt("Skill Use Restrictions", (int*) &AtkSkill.GroundAirBoth, 0, 2, items[AtkSkill.GroundAirBoth]);
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
        InputShort("Projectile Behaviour ID", &AtkSkill.ProjectileBehaviour);
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

    if (ImGui::Button("Close")) 
    {
        RenderAtkSkillWindow = false; // Deactivates the window.
    }
    ImGui::PopStyleVar();
    ImGui::End();
}

void HexEditorWindow(short Idx)
{
    hex_edit.ReadOnly = false;
    hex_edit.DrawWindow("Hex Editor", &skillarray[4], 144);
}

void SkillPackWindow()
{
    ImGui::Begin("Enter a name for your skill pack: ");
    ImGui::InputText("Skill Pack Name", packname, 32);

    if (ImGui::Button("Save")) 
    {
        if (SUCCEEDED(FileSaveDialog(skillpack, L".bin")))
        {
            SaveSkillPack();
            RenderSkillPackWindow = false;
        }
    }

    ImGui::End();
}
