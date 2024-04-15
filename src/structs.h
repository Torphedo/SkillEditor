#pragma once
#include <string>

#include <stdint.h>

typedef enum {
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
    uint32_t unkint;
    uint16_t unk0;
    uint16_t unk1;
    uint16_t SkillTextID;
    uint16_t unk2;
    uint16_t RegisterID;
    uint16_t SkillID;
    uint16_t RarityStars; // n + 1
    uint16_t unk3;
    uint16_t SoundFileID;
    uint16_t CapsuleType; // Aura, Atk, Def, etc.
    uint16_t unk4; // Pursuit has 01 for this.
    uint16_t SchoolID;
    uint16_t AnimationProfileGround;
    uint16_t AnimationProfileAir;
    uint16_t MultiPress1;
    uint16_t MultiPress2;
    uint16_t DoubleSkill1;
    uint16_t DoubleSkill2;
    uint16_t unk5;
    uint16_t PostHitSFX;
    uint16_t StartUpSFX;
    uint16_t CollisionSFX;
    uint16_t Cost;
    uint16_t CostEffect;
    uint16_t ExtraCost;
    uint16_t HealthCost;
    uint16_t SkillUses;
    uint16_t SkillUsageUnk1;
    uint16_t SkillUsageUnk2;
    uint16_t SkillUsageUnk3;
    uint16_t unk6;
    uint16_t unk7;
    uint16_t unk8;
    uint16_t unk9;
    uint16_t unk10;
    uint16_t unk11;
    uint16_t SelfEffect;
    uint16_t ButtonRestrictions;
    uint16_t Requirements;
    uint16_t ReqAmount;
    uint16_t GroundAirBoth; // 00 00, 01 00, or 02 00
    uint16_t SkillButtonEffect; // ???
    uint16_t unk12;
    uint16_t AppliedStatusID;
    uint16_t Restriction;
    uint16_t StrengthEffect; // ???
    uint16_t Damage;
    uint16_t EffectDuration; // Misc effects?
    uint16_t TargetHand; // Hand-in-hand with HitEffectSkills
    uint16_t unk13;
    uint16_t HitEffectSkills; // 01 = Erase skill on hit. Only used on Lightning Sword in vanilla.
    uint16_t Increase; // 09 = Health, 0B = Lvl (Increase = atk) // Increase stat by the damage value
    uint8_t StatusEnabler; // Status ID won't apply unless this is 2C
    uint8_t StatusDuration; // Status ID Duration
    uint16_t unk14;
    uint16_t ProjectileProperties; // 02 = Penetrate Defense
    uint16_t ProjectileID; // See spreadsheet.
    uint16_t CollisionSkillID; // ???
    uint16_t HomingRangeFirstHit;  // First hit
    uint16_t HomingRangeSecondHit; // Knock down (if 0, no stagger at all)
    uint16_t HomingRangeThirdHit;  // Combo end (removing this makes infinite range)
    uint16_t unk15;
    uint16_t unk16;
    uint16_t unk17;
    uint16_t ProjectileBehaviour; // Downed Hit Animation or Projectile Behaviour
    uint8_t SkillDuration;
    uint8_t hitbox_size;
    uint16_t ExpandSkillWidth;
    uint16_t AnimationSize;
    uint16_t ProjectileSpeed; // Can also be projectile rain count
    uint16_t AccuracyID;
    uint16_t AnimationHeight;
}atkskill;

typedef struct gsdata {
    uint32_t filesize; // The size in bytes of the entire gsdata file
    uint32_t unk0;// TBD
    uint32_t unk1;// TBD
    uint32_t unk2;// TBD
    uint32_t VersionNum; // Decimal on title screen is placed 2 digits from the right: (3947602715 -> 39476027.15)
    uint32_t skill_limiter; // The number of skills allowed (default 0x176, 0d374) TODO: Improve this description
    uint8_t dummy[136]; // This is actual data, but it's unimportant to us and gets ignored.
    atkskill skill_array[751];
}gsdata;

// Original format, skill data only
typedef struct skill_pack_header_v1 {
    char name[32];
    uint16_t format_version; // 1
    uint16_t skill_count;
    uint8_t pad[12];
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
