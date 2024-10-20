#pragma once

#include "types.h"

typedef enum {
    AURA,
    ATTACK,
    DEFENSE,
    ERASE,
    ENVIROMENTAL,
    STATUS,
    SPECIAL
}capsule;

const uintptr_t gstorage_offset = 0x4C5240;

typedef struct {
    u32 unkint;
    u16 unk0;
    u16 unk1;
    u16 SkillTextID;
    u16 unk2;
    u16 RegisterID;
    u16 SkillID;
    u16 RarityStars; // n + 1
    u16 unk3;
    u16 SoundFileID;
    u16 CapsuleType; // Aura, Atk, Def, etc.
    u16 unk4; // Pursuit has 01 for this.
    u16 SchoolID;
    u16 AnimationProfileGround;
    u16 AnimationProfileAir;
    u16 MultiPress1;
    u16 MultiPress2;
    u16 DoubleSkill1;
    u16 DoubleSkill2;
    u16 unk5;
    u16 PostHitSFX;
    u16 StartUpSFX;
    u16 CollisionSFX;
    u16 Cost;
    u16 CostEffect;
    u16 ExtraCost;
    u16 HealthCost;
    u16 SkillUses;
    u16 SkillUsageUnk1;
    u16 SkillUsageUnk2;
    u16 SkillUsageUnk3;
    u16 unk6;
    u16 unk7;
    u16 unk8;
    u16 unk9;
    u16 unk10;
    u16 unk11;
    u16 SelfEffect;
    u16 ButtonRestrictions;
    u16 Requirements;
    u16 ReqAmount;
    u16 GroundAirBoth; // 00 00, 01 00, or 02 00
    u16 SkillButtonEffect; // ???
    u16 unk12;
    u16 AppliedStatusID;
    u16 Restriction;
    u16 StrengthEffect; // ???
    u16 Damage;
    u16 EffectDuration; // Misc effects?
    u16 TargetHand; // Hand-in-hand with HitEffectSkills
    u16 unk13;
    u16 HitEffectSkills; // 01 = Erase skill on hit. Only used on Lightning Sword in vanilla.
    u16 Increase; // 09 = Health, 0B = Lvl (Increase = atk) // Increase stat by the damage value
    u8 StatusEnabler; // Status ID won't apply unless this is 2C
    u8 StatusDuration; // Status ID Duration
    u16 unk14;
    u16 ProjectileProperties; // 02 = Penetrate Defense
    u16 ProjectileID; // See spreadsheet.
    u16 CollisionSkillID; // ???
    u16 HomingRangeFirstHit;  // First hit
    u16 HomingRangeSecondHit; // Knock down (if 0, no stagger at all)
    u16 HomingRangeThirdHit;  // Combo end (removing this makes infinite range)
    u16 unk15;
    u16 unk16;
    u16 unk17;
    u16 ProjectileBehaviour; // Downed Hit Animation or Projectile Behaviour
    u8 SkillDuration;
    u8 hitbox_size;
    u16 ExpandSkillWidth;
    u16 AnimationSize;
    u16 ProjectileSpeed; // Can also be projectile rain count
    u16 AccuracyID;
    u16 AnimationHeight;
}atkskill;

// Original format, skill data only
typedef struct skill_pack_header_v1 {
    char name[32];
    u16 format_version; // 1
    u16 skill_count;
    u8 pad[12];
}pack_header1;

// Second format, skill and text data
typedef struct skill_pack_v2_text {
    u16 name_length; // Should be calculated with strlen()
    u16 desc_length;
}pack2_text;

typedef struct {
    char* str;
    u16 len;
}sized_str;

typedef struct text_header {
    u32 text_size; // Size of the header, offset table, and all skill text
    u32 offset_count;
    u32 offset_table_size; // offset_count * sizeof(text_ptrs)
    u32 version_num; // Duplicate from header?
    u32 skill_limiter; // Duplicate from header?
}text_header;

typedef struct text_ptrs {
    u32 index;
    u32 name;
    u32 desc;
}text_ptrs;

enum {
    GSDATA_PADDING_SIZE = 0x198F4,
    GSDATA_TEXTPTR_COUNT = 393,
    GSDATA_TEXTBUF_SIZE = 56504,
};

typedef struct gsdata {
    u32 filesize; // The size in bytes of the entire gsdata file
    u32 unk0;// TBD
    u32 unk1;// TBD
    u32 unk2;// TBD
    u32 VersionNum; // Decimal on title screen is placed 2 digits from the right: (3947602715 -> 39476027.15)
    u32 skill_limiter; // The number of skills allowed (default 0x176, 0d374) TODO: Improve this description
    u8 dummy[136]; // This is actual data, but it's unimportant to us and gets ignored.
    atkskill skill_array[751];
    u8 pad[GSDATA_PADDING_SIZE];
    text_header textHeader;
    text_ptrs textPtrs[GSDATA_TEXTPTR_COUNT];
    char textbuf[GSDATA_TEXTBUF_SIZE];
}gsdata;

