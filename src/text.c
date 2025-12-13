#include <string.h>
#include <assert.h>
#include <windows.h>

#include <common/int.h>
#include <common/logging.h>

#include "remote_pd.h"
#include "text.h"

void expand_text_table(pd_meta p, s16 new_size) {
    gsdata* gstorage = p.gstorage.local_data;
    if (new_size > 5000) {
        LOG_MSG(warning, "[Programmer error] New max size (%d) seems unreasonable, I'm going to ignore it.\n");
        return;
    }

    text_header* header = &gstorage->textHeader;
    const u32 cur_size = header->offset_count;
    // How many new entries to add to the offset table
    const s64 diff = new_size - cur_size;
    if (diff < 0) {
        LOG_MSG(warning, "[Programmer error] New max ID (%d) < old max ID (%d). This doesn't make sense, I'm going to ignore it.\n");
        return;
    }

    // How many bytes to shift the text buffer by
    const s64 byte_diff = diff * sizeof(text_ptrs);
    u8* textbuf = (u8*)&gstorage->textPtrs[cur_size];
    u8* text_target = textbuf + byte_diff;

    // Shift text buffer forward to make space
    const u32 textbuf_size = header->text_size - header->offset_count - sizeof(*header);
    memmove(text_target, textbuf, textbuf_size);
    // Wipe the newly unused space
    memset(textbuf, 0, byte_diff);

    for (u32 i = 0; i < cur_size; i++) {
        gstorage->textPtrs[i].name += byte_diff;
        gstorage->textPtrs[i].desc += byte_diff;
    }

    for (u32 i = cur_size; i < new_size; i++) {
        text_ptrs* entry = &gstorage->textPtrs[i];
        const text_ptrs* prev = &gstorage->textPtrs[i - 1];
        const char* prev_desc = ((u8*)prev) + prev->desc;
        entry->index = prev->index + 1;
        entry->name = prev->desc + 1 + (strlen(prev_desc) + 1);
        entry->desc = entry->name + 1;
    }

    header->offset_count = new_size;
    header->offset_table_size += byte_diff;

    skill_text text = get_skill_text(p, new_size - 1);
    const u8* textbuf_end = (u8*)(text.desc + strlen(text.desc) + 1);
    header->text_size = textbuf_end - (u8*)header;
}

skill_text get_skill_text(pd_meta p, unsigned int id) {
    gsdata* gstorage = p.gstorage.local_data;

    // Read section header to check the number of strings
    const text_header* header = &gstorage->textHeader;
    if (header->offset_count <= id) {
        expand_text_table(p, id + 5);
    }

    if (id > header->offset_count - 1) {
        return (skill_text) {"", ""};
    }

    const text_ptrs* string_offset = &gstorage->textPtrs[id];

    const char* name = ((char*)string_offset) + string_offset->name;
    const char* desc = ((char*)string_offset) + string_offset->desc;

    skill_text output = { name, desc };

    return output;
}

/// @brief Resize the name or description at a specific text ID
///
/// This moves the rest of the string pool as needed.
/// @param p PD object to access GSDATA through
/// @param offender Pointer to the string to be resized
/// @param new_len The new length of the string
/// @param offender_id The text ID of the string
/// @param is_name Whether the string is a name (and not a description)
void shift_textbuf(pd_meta p, char* offender, s32 new_len, u32 offender_id, bool is_name) {
    const s64 diff = new_len - (s64)(strlen(offender) + 1);
    gsdata* gstorage = p.gstorage.local_data;
    if (diff <= 0 || offender < (char*)gstorage) {
        return;
    }
    gstorage->textHeader.text_size += diff + 1;

    const ptrdiff_t offender_pos = (u8*)offender - (u8*)&gstorage->textHeader;
    const ptrdiff_t remaining_space = gstorage->textHeader.text_size - offender_pos;
    assert(remaining_space > 0);
    char* target = offender + strlen(offender) + 1; // Start of the data to move
    memmove(target + diff, target, remaining_space);
    memset(target, 0, diff); // Clear the new space

    for (u32 i = offender_id + 1; i < gstorage->textHeader.offset_count; i++) {
        text_ptrs* offsets = &gstorage->textPtrs[i];
        offsets->name += diff;
        offsets->desc += diff;
    }

    // If we update the position of a name, the description's offset needs to update too.
    if (is_name) {
        text_ptrs* offsets = &gstorage->textPtrs[offender_id];
        offsets->desc += diff;
    }
}

bool save_skill_text(pd_meta p, skill_text text, unsigned int id) {
    gsdata* gstorage = p.gstorage.local_data;
    const text_header* header = &gstorage->textHeader;
    if (header->offset_count <= id) {
        expand_text_table(p, id + 5);
    }

    // Get offset data
    const u16 index = id;
    const text_ptrs* string_offset = &gstorage->textPtrs[index];

    // Shorthands for the new text
    const char* new_name = text.name;
    const char* new_desc = text.desc;
    const s32 name_size = strlen(text.name) + 1;
    const s32 desc_size = strlen(text.desc) + 1;

    char* name = (char*)string_offset + string_offset->name;
    shift_textbuf(p, name, name_size, index, true);

    // IMPORTANT: This must come after the above shift_textbuf call. If the name
    // string changes size, the description offset will be different.
    char* desc = (char*)string_offset + string_offset->desc;
    shift_textbuf(p, desc, desc_size, index, false);

    // Apply text changes to the string pool
    memcpy((void*)name, new_name, name_size);
    memcpy((void*)desc, new_desc, desc_size);

    return true;
}