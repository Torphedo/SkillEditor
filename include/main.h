#pragma once

#include <stdint.h>
#include <string>
#include <imgui_internal.h>

using namespace std;

string filepath;
char* filepathptr;
fstream AtkSkillFile;
const COMDLG_FILTERSPEC fileTypes[] =
{
    { L"Skill File", L"*.skill;" },
};

void LoadAttackSkill(char* filename);

typedef struct AttackSkill
{
    int unkint;
    short unk0;
    short unk1;
    short SkillTextID;
    short unk2;
    short RegisterID;
    short SkillID;
    short RarityStars; // n + 1
    short unk3;
    short SoundFileID;
    short CapsuleType; // Aura, Atk, Def, etc.
    short unk4; // Pursuit has 01 for this.
    short SchoolID;
    short AnimationIDGround;
    short AnimationIDAir;
    short MultiPress1;
    short MultiPress2;
    short DoubleSkill1;
    short DoubleSkill2;
    short unk5;
    short PostHitSFX;
    short StartUpSFX;
    short CollisionSFX;
    short Cost;
    short CostEffect;
    short ExtraCost;
    short HealthCost;
    short SkillUses;
    short SkillUsageUnk1;
    short SkillUsageUnk2;
    short SkillUsageUnk3;
    short unk6;
    short unk7;
    short unk8;
    short unk9;
    short unk10;
    short unk11;
    short SelfEffect;
    short ButtonRestrictions;
    short Requirements;
    short ReqAmount;
    short GroundAirBoth; // 00 00, 01 00, or 02 00
    short SkillButtonEffect; // ???
    short unk12;
    short AppliedStatusID;
    short Restriction;
    short StrengthEffect; // ???
    short Damage;
    short EffectDuration; // Misc effects?
    short TargetHand; // Hand-in-hand with HitEffectSkills
    short unk13;
    short HitEffectSkills; // 01 = Erase skill on hit. Only used on Lightning Sword in vanilla.
    short Increase; // 09 = Health, 0B = Lvl (Increase = atk) // Increase stat by the damage value
    uint8_t StatusEnabler; // Status ID won't apply unless this is 2C
    uint8_t StatusDuration; // Status ID Duration
    short unk14;
    short ProjectileProperties; // 02 = Penetrate Defense
    short ProjectileID; // See spreadsheet.
    short CollisionSkillID; // ???
    short HomingRangeFirstHit;  // First hit
    short HomingRangeSecondHit; // Knock down (if 0, no stagger at all)
    short HomingRangeThirdHit;  // Combo end (removing this makes infinite range)
    short unk15;
    short unk16;
    short unk17;
    short unknown; // Downed Hit Animation or Projectile Behaviour
    uint8_t SkillDuration;
    uint8_t HitRange;
    short ExpandSkillWidth;
    short AnimationSize;
    short ProjectileSpeed; // Can also be projectile rain count
    short AccuracyID;
    short AnimationHeight;
}atkskill;

void InputShort(const char *label, void *p_data) {
    const short s16_one = 1;
    ImGui::InputScalar(label, ImGuiDataType_S16, p_data, true ? &s16_one : NULL, NULL, "%d");
}

void InputUInt8(const char* label, void* p_data) {
    const short s8_one = 1;
    ImGui::InputScalar(label, ImGuiDataType_S8, p_data, true ? &s8_one : NULL, NULL, "%d");
}


string PWSTR_to_string(PWSTR ws) {
    string ret;
    ret.reserve(wcslen(ws));
    for (; *ws; ws++)
        ret += (char)*ws;
    return ret;
}

int WINAPI FileSelectDialog(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        filepath = PWSTR_to_string(pszFilePath);
                        filepathptr = const_cast<char*>(filepath.c_str());
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return 0;
}

int WINAPI FileSaveDialog(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileSaveDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
            IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);
            hr = pFileOpen->SetDefaultExtension(L".skill");
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        filepath = PWSTR_to_string(pszFilePath);
                        filepathptr = const_cast<char*>(filepath.c_str());
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return 0;
}
