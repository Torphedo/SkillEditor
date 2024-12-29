#include <cstring>
#include <windows.h>

#include <common/int.h>

#include "remote_pd.hxx"
#include "structs.h"
#include "text.hxx"

skill_text load_skill_text(pd_meta p, unsigned int id) {
    if (p.h == INVALID_HANDLE_VALUE || p.h == NULL) {
        return {"", ""};
    }

    // Read section header to check the number of strings
    const text_header* header = &p.gstorage->textHeader;

    if (id > header->offset_count - 1) {
        return {"", ""};
    }

    const text_ptrs* string_offset = &p.gstorage->textPtrs[id - 1];

    const char* name = ((char*)string_offset) + string_offset->name;
    const char* desc = ((char*)string_offset) + string_offset->desc;

    skill_text output = { name, desc };

    return output;
}

void shift_textbuf(pd_meta p, char* offender, u32 new_len, u32 offender_id, bool is_name) {
    const s32 diff = new_len - (strlen(offender) + 1);
    if (diff <= 0 || offender < (char*)p.gstorage) {
        return;
    }

    const u64 remaining_space = sizeof(p.gstorage->textbuf) - (u64)(offender - p.gstorage->textbuf);
    char* target = offender + strlen(offender) + 1; // Start of the data to move
    memmove(target + diff, target, remaining_space);
    memset(target, 0, diff); // Clear the new space

    for (int i = offender_id + 1; i < ARRAYSIZE(p.gstorage->textPtrs); i++) {
        text_ptrs* offsets = &p.gstorage->textPtrs[i];
        offsets->name += diff;
        offsets->desc += diff;
    }

    // If we update the position of a name, the description's offset needs to update too.
    if (is_name) {
        text_ptrs* offsets = &p.gstorage->textPtrs[offender_id];
        offsets->desc += diff;
    }
}

bool save_skill_text(pd_meta p, skill_text text, unsigned int id) {
    if (p.h == INVALID_HANDLE_VALUE) {
        return false;
    }

    const text_header* header = &p.gstorage->textHeader;
    if (id > header->offset_count - 1) {
        return false;
    }

    // Get offset data
    const text_ptrs* string_offset = &p.gstorage->textPtrs[id - 1];

    // Shorthands for the new text
    const char* new_name = text.name.data();
    const char* new_desc = text.desc.data();
    const u32 name_size = strlen(new_name) + 1;
    const u32 desc_size = strlen(new_desc) + 1;

    char* name = ((char*)&p.gstorage->textPtrs) + string_offset->name;
    shift_textbuf(p, name, name_size, id - 1, true);

    // IMPORTANT: This must come after the above shift_textbuf call. If the name
    // string changes size, the description offset will be different.
    char* desc = ((char*)&p.gstorage->textPtrs) + string_offset->desc;
    shift_textbuf(p, desc, desc_size, id - 1, false);

    // Apply text changes to the string pool
    memcpy((void*)name, new_name, name_size);
    memcpy((void*)desc, new_desc, desc_size);

    return true;
}