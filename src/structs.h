#pragma once

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
    short AnimationProfileGround;
    short AnimationProfileAir;
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
    short ProjectileBehaviour; // Downed Hit Animation or Projectile Behaviour
    uint8_t SkillDuration;
    uint8_t HitRange;
    short ExpandSkillWidth;
    short AnimationSize;
    short ProjectileSpeed; // Can also be projectile rain count
    short AccuracyID;
    short AnimationHeight;
}atkskill;

struct GSDataHeader
{
    int Filesize; // The size in bytes of the entire gsdata file
    int unk0;// TBD
    int unk1;// TBD
    int unk2;// TBD
    int VersionNum; // Decimal on title screen is placed 2 digits from the right: (3947602715 -> 39476027.15)
    int SkillLimiter; // The number of skills allowed (default 0x176, 0d374) TODO: Improve this description
    char dummy[136]; // This is actual data, but it's unimportant so it gets ignored.
};

typedef struct SkillPackHeaderV1
{
    char Name[32];
    short FormatVersion;
    short SkillCount;
    char pad[12];
}packheader1;
