#pragma once
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
    GSDATA_PADDING_SIZE = 0x198F4,
    GSDATA_TEXTPTR_COUNT = 393,
    GSDATA_TEXTBUF_SIZE = 56504,
    GSDATA_SKILL_COUNT = 751
};

typedef struct {
    // This could be made a big array of 0x90 bytes, but the ID & text ID are
    // needed all over the place.

    u8 data1[0x8];
    // SkillTextID offset = 0x8
    u16 SkillTextID;
    u8 data2[0x4];
    // SkillID offset = 0xE
    u16 SkillID;

    u8 data3[0x80];
}skill_t;

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
const uintptr_t gstorage_offset = 0x4C5240;

typedef struct {
    u32 filesize; // The size in bytes of the entire gsdata file
    u32 unk0; // TBD
    u32 unk1; // TBD
    u32 unk2; // TBD
    u32 VersionNum; // Decimal on title screen is placed 2 digits from the right: (3947602715 -> 39476027.15)
    u32 skill_limiter; // The number of skills allowed (default 0x176, 0d374) TODO: Improve this description
    u8 dummy[136]; // This is actual data, but it's un-researched so we ignore it.
    skill_t skill_array[GSDATA_SKILL_COUNT];
    u8 pad[GSDATA_PADDING_SIZE];
    text_header textHeader;
    text_ptrs textPtrs[GSDATA_TEXTPTR_COUNT];
    char textbuf[GSDATA_TEXTBUF_SIZE];
}gsdata;