#pragma once
#include <stddef.h>
#include <common/int.h>

// =============================================================================
// Official PD structures, all field names unofficial.

typedef struct {
    u32 text_size; // Size of the header, offset table, and all skill text
    u32 offset_count;
    u32 offset_table_size; // offset_count * sizeof(text_ptrs)
    u32 version_num; // Duplicate from header?
    u32 skill_limiter; // Duplicate from header?
}text_header;

typedef struct {
    u32 index;
    u32 name;
    u32 desc;
}text_ptrs;

// Some of these could change if/when new skills are added
enum {
    GSDATA_PADDING_SIZE = 0x198EC,
    GSDATA_SKILL_COUNT = 752,
    GSDATA_SIZE = 0x44004,
    ANIMATION_PROFILE_COUNT = GSDATA_SKILL_COUNT,
};

typedef struct {
    u16 SkillTextID;
    u8 data2[0x4];
    u16 SkillID;
    u16 RegisterID;
    u8 data3[0x4];
    u16 CapsuleType;
    u8 data4[0x4];
    u16 AnimProfileGround;
    u16 AnimProfileAir;

    u8 data5[0x78];
}skill_t;
static_assert(sizeof(skill_t) == 0x90, "Skill size is wrong!");
// CLion complains about this because it interprets it as C++, you can ignore the linter warnings.
static_assert(offsetof(skill_t, SkillTextID) == 0x0, "Skill text ID offset is wrong!");
static_assert(offsetof(skill_t, SkillID) == 0x6, "Skill ID offset is wrong!");
static_assert(offsetof(skill_t, AnimProfileGround) == 0x14, "Ground animation profile offset is wrong!");
static_assert(offsetof(skill_t, AnimProfileAir) == 0x16, "Air animation profile offset is wrong!");

typedef enum {
    AURA,
    ATTACK,
    DEFENSE,
    ERASE,
    ENVIROMENTAL,
    STATUS,
    SPECIAL
}capsule;

// The gsdata structure is at this offset in PDUWP.exe
static const uintptr_t gstorage_offset = 0x4C5240;
// The offset 0x4BCC98 has also been associated with the animation profiles
static const uintptr_t anim_profiles_offset = 0x4E024C;

typedef struct {
    u32 filesize; // The size in bytes of the entire gsdata file
    u32 unk0; // TBD
    u32 unk1; // TBD
    u32 unk2; // TBD
    u32 VersionNum; // Decimal on title screen is placed 2 digits from the right: (3947602715 -> 39476027.15)
    u32 skill_limiter; // The number of skills allowed (default 0x176, 0d374) TODO: Improve this description
    skill_t skill_array[GSDATA_SKILL_COUNT];
    u8 pad[GSDATA_PADDING_SIZE];
    text_header textHeader;
    text_ptrs textPtrs[];
}gsdata;
static_assert(offsetof(gsdata, skill_array) == 0x18, "Skill array offset is wrong");
static_assert(offsetof(gsdata, textHeader) == 0x34004, "Text header offset is wrong");
static_assert(offsetof(gsdata, textPtrs) == 0x34018, "Text pointers offset is wrong");

typedef struct {
    u8 data[0x72];
}anim_profile;
static_assert(sizeof(anim_profile) == 0x72);