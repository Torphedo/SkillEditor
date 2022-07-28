#pragma once

#include <stdint.h>
#include <string>

using namespace std;

string filepath;
char* filepathptr;

void LoadAttackSkill(char* filename);

string PWSTR_to_string(PWSTR ws) {
    string ret;
    ret.reserve(wcslen(ws));
    for (; *ws; ws++)
        ret += (char)*ws;
    return ret;
}

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
    short unk12;
    short Requirements;
    short ReqAmount;
    short GroundAirBoth; // 00 00, 01 00, or 02 00
    short SkillButtonEffect; // ???
    short unk13;
    short AppliedStatusID;
    short Restriction;
    short StrengthEffect;
    short Damage;
    short EffectDuration; // Misc effects?
    short TargetHand; // Hand-in-hand with HitEffectSkills
    short unk14;
    short HitEffectSkills; // 01 = Erase skill on hit. Only used on Lightning Sword in vanilla.
    short Increase; // 09 = Health, 0B = Lvl (Increase = atk)
    uint8_t StatusEnabler; // Status ID won't apply unless this is 2C
    uint8_t StatusDuration; // Status ID Duration
    short unk15;
    short ProjectileProperties; // 02 = Penetrate Defense
    short ProjectileID; // See spreadsheet.
    short CollisionSkillID; // ???
    short HomingRangeFirstHit;  // First hit
    short HomingRangeSecondHit; // Knock down (if 0, no stagger at all)
    short HomingRangeThirdHit;  // Combo end (removing this makes infinite range)
    short unk16;
    short unk17;
    short unk18;
    short unknown; // Downed Hit Animation
    uint8_t SkillDuration;
    uint8_t HitRange;
    short ExpandSkillWidth;
    short AnimationSize;
    short ProjectileSpeed; // Can also be projectile rain count
    short AccuracyID;
    short AnimationHeight;
}atkskill;

