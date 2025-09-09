#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "remote_pd.h"

// All the text associated with a skill. These are views into the real string
// buffer, so copy them into a std::string if you need to change the size.
typedef struct {
    const char* name;
    const char* desc;
} skill_text;

// Finds the name/description of the provided skill text ID
skill_text get_skill_text(pd_meta p, unsigned int id);

// Writes skill text to memory
bool save_skill_text(pd_meta p, skill_text text, unsigned int id);

#ifdef __cplusplus
}
#endif