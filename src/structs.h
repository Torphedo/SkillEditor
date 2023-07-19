#pragma once
#include <stdint.h>

typedef enum : uint16_t {
    AURA,
    ATTACK,
    DEFENSE,
    ERASE,
    ENVIROMENTAL,
    STATUS,
    SPECIAL
}capsule_t;

const uintptr_t gstorage_offset = 0x4C5240;

typedef struct {
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
    capsule_t CapsuleType; // Aura, Atk, Def, etc.
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
    uint8_t hitbox_size;
    short ExpandSkillWidth;
    short AnimationSize;
    short ProjectileSpeed; // Can also be projectile rain count
    short AccuracyID;
    short AnimationHeight;
}atkskill;

typedef struct gsdata {
    unsigned int filesize; // The size in bytes of the entire gsdata file
    unsigned int unk0;// TBD
    unsigned int unk1;// TBD
    unsigned int unk2;// TBD
    unsigned int VersionNum; // Decimal on title screen is placed 2 digits from the right: (3947602715 -> 39476027.15)
    unsigned int skill_limiter; // The number of skills allowed (default 0x176, 0d374) TODO: Improve this description
    char dummy[136]; // This is actual data, but it's unimportant to us and gets ignored.
    atkskill skill_array[751];
}gsdata;

extern gsdata gstorage;

// Original format, skill data only
typedef struct skill_pack_header_v1 {
    char name[32];
    short format_version; // 1
    short skill_count;
    char pad[12];
}pack_header1;

// Second format, skill and text data
typedef struct skill_pack_v2_text {
    uint16_t name_length; // Should be calculated with strlen()
    uint16_t desc_length;
}pack2_text;

typedef struct skill_text {
    std::string name;
    std::string desc;
}skill_text;

typedef struct text_header {
    unsigned char unknown[8];
    uint32_t array_size;
    uint32_t unknown2;
    uint32_t unknown_skill_count;
    uint32_t skill_count;
}text_header;

typedef struct text_ptrs {
    uint32_t index;
    uint32_t name;
    uint32_t desc;
}text_ptrs;
