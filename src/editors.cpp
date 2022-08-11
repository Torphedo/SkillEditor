// Editor windows, custom functions to be triggered by the UI, and other miscellaneous
// code that doesn't need to be in the main UI window file (UI.cpp)

#include <main.h>
#include <editors.h>

using std::cout;

short ErrorCode;
bool OptionsWindow = false;
bool RenderSkillPackWindow = false;
short AtkSkillState = 0; // 0 = None, 1 = Opened, 2 = Saved
bool RenderAtkSkillWindow = false;

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

void AtkSkillWindow()
{
    std::string WindowTitle = "Attack Skill Editor - " + filepath; // Use filename in the window title.
    ImGui::Begin(const_cast<char*>(WindowTitle.c_str()));     // TODO: chop off the filepath and use only the name.

    InputShort("Skill Text ID", &AtkSkill.SkillTextID);
    Tooltip("The skill ID from which to get the skill's name,\ndescription, etc.");
    InputShort("Register ID", &AtkSkill.RegisterID);
    Tooltip("The \"register\" / \"library\" of skills this skill\nwill belong to.");
    InputShort("Skill ID", &AtkSkill.SkillID);
    Tooltip("The skill's internal ID. This will determine what\nskill (if any) will be overwritten. This ID has no\nrelation to the IDs seen in-game.");

    char* rarity_txt[] = { "1","2","3","4","5" };
    ImGui::Combo("Rarity", (int*) &AtkSkill.RarityStars, rarity_txt, 5);
    Tooltip("The skill's in-game rarity, displayed as stars.");

    InputShort("Sound File ID", &AtkSkill.SoundFileID);
    InputShort("Capsule Type", &AtkSkill.CapsuleType);

    // char* capsule_types[] = {"Aura Particle","Attack","Defense","Erase","Environmental","Status","Special"};
    // ImGui::Combo("Capsule Type", &Idx, capsule_types, IM_ARRAYSIZE(capsule_types), IM_ARRAYSIZE(capsule_types));
    Tooltip("The skill's type. (Attack, Defense, Environmental, etc.)");

    InputShort("School ID", &AtkSkill.SchoolID);
    Tooltip("The skill's school. (Nature, Optical, Ki, etc.)");
    InputShort("Animation Profile (Ground)", &AtkSkill.AnimationProfileGround);
    Tooltip("This controls the player's skeletal animation, the number\nof projectiles fired, particle effects used, and much more.\nThis profile is used when the skill is cast on the ground.");
    InputShort("Animation Profile (Air)", &AtkSkill.AnimationProfileAir);
    Tooltip("This controls the player's skeletal animation, the number\nof projectiles fired, particle effects used, and much more.\nThis profile is used when the skill is cast in the air.");
    InputShort("Multi Press 1", &AtkSkill.MultiPress1);
    InputShort("Multi Press 2", &AtkSkill.MultiPress2);
    InputShort("Double Skill 1", &AtkSkill.DoubleSkill1);
    InputShort("Double Skill 2", &AtkSkill.DoubleSkill2);
    InputShort("After Hit SFX", &AtkSkill.PostHitSFX);
    InputShort("Start Up SFX", &AtkSkill.StartUpSFX);
    Tooltip("The sound effect ID to be played when the skill\nis winding up / charging.");
    InputShort("Collision SFX", &AtkSkill.CollisionSFX);
    Tooltip("The sound effect ID to be played when the skill\ncollides with something.");
    InputShort("Aura Cost", &AtkSkill.Cost);
    Tooltip("The amount of Aura the skill costs.");
    InputShort("Additional Aura Cost", &AtkSkill.ExtraCost);
    Tooltip("Adds to the base Aura cost.");
    InputShort("Health Penalty", &AtkSkill.HealthCost);
    Tooltip("The amount of health to be taken from the user\nwhen the skill is cast.");
    InputShort("# of Uses", &AtkSkill.SkillUses);
    Tooltip("How many times the skill can be used. Set this\nto 0 for infinite uses.");
    InputShort("Self Effect", &AtkSkill.SelfEffect);
    InputShort("Button Restrictions", &AtkSkill.ButtonRestrictions);
    InputShort("Requirements", &AtkSkill.Requirements);
    Tooltip("The skill's type of special requirement.\n0 = None\n1 = Health\n5 = # of Skills Remaining in Deck\n7 = Aura\n9 = Level");
    InputShort("Requirement Amount", &AtkSkill.ReqAmount);
    Tooltip("The required amount of the type specified above");

    char* items[] = { "Ground","Air","Both" };
    ImGui::Combo("Air/Ground/Both", (int*)&AtkSkill.GroundAirBoth, items, IM_ARRAYSIZE(items));
    Tooltip("Whether the skill can be used on\nthe ground, in the air, or both.");

    InputShort("Skill Button Effect", &AtkSkill.SkillButtonEffect);
    InputShort("Applied Status ID", &AtkSkill.AppliedStatusID);
    InputShort("Restrictions", &AtkSkill.Restriction);
    InputShort("Strength Effect", &AtkSkill.StrengthEffect);
    InputShort("Damage", &AtkSkill.Damage);
    InputShort("Effect Duration / Misc. Effects", &AtkSkill.EffectDuration);
    InputShort("Target Hand Data", &AtkSkill.TargetHand);
    InputShort("Hit Effect Skills", &AtkSkill.HitEffectSkills);
    InputShort("Increase Stat", &AtkSkill.Increase);
    InputUInt8("Status Enabler", &AtkSkill.StatusEnabler);
    InputUInt8("Status ID Duration", &AtkSkill.StatusDuration);
    InputShort("Projectile Properties", &AtkSkill.ProjectileProperties);
    InputShort("Projectile ID", &AtkSkill.ProjectileID);
    InputShort("\"Collision Skill ID\"", &AtkSkill.ProjectileID);
    InputShort("Homing Range 1st Hit", &AtkSkill.HomingRangeFirstHit);
    InputShort("Knockback Strength", &AtkSkill.HomingRangeSecondHit);
    InputShort("Combo End", &AtkSkill.HomingRangeThirdHit);
    InputUInt8("Skill Duration", &AtkSkill.SkillDuration);
    InputUInt8("Hit Range", &AtkSkill.HitRange);
    InputShort("Expand Skill Width / Start Speed", &AtkSkill.ExpandSkillWidth);
    InputShort("Animation Size / Acceleration", &AtkSkill.AnimationSize);
    InputShort("Projectile End Speed", &AtkSkill.ProjectileSpeed);
    InputShort("Homing Strength / Accuracy", &AtkSkill.AccuracyID);
    InputShort("Animation Height", &AtkSkill.AnimationHeight);

    if (ImGui::Button("Close")) 
    {
        RenderAtkSkillWindow = false; // Deactivates the window.
    }
    ImGui::End();
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
