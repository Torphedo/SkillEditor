#include "loader.hxx"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <common/int.h>
#include <common/logging.h>
#include "../../src/mods.hxx"

pd_meta get_local_pd_meta(u8* pduwp = nullptr) {
    if (!pduwp) {
        pduwp = (u8*)GetModuleHandle(nullptr);
    }
    auto* gstorage = (gsdata*)(pduwp + gstorage_offset);
    auto* anims = (anim_profile*)(pduwp + anim_profiles_offset);

    pd_meta p = {};
    p.gstorage.local_data = gstorage;
    p.gstorage.size = GSDATA_SIZE;
    p.anim_profiles.local_data = anims;
    p.anim_profiles.size = ANIMATION_PROFILE_COUNT * sizeof(anim_profile);

    return p;
}

void load_skills() {
    if (!PHYSFS_exists("skills")) {
        PHYSFS_mkdir("skills");
    }

    char** files = PHYSFS_enumerateFiles("skills");

    pd_meta p = get_local_pd_meta();
    std::string real_root = PHYSFS_getRealDir("/");
    for (char** i = files; *i != nullptr; i++) {
        std::string path = real_root + "/skills/" + *i;
        install_mod(p, &path, 1);
    }
}

int PLUGIN_API command_load_skill(int argc, char** argv) {
    printf("You asked to load a skill.\n");
    pd_meta p = get_local_pd_meta();

    for (u32 i = 1; i < argc; i++) {
        std::string path = argv[i];
        LOG_MSG(info, "Loading skill file '%s'\n", path.c_str());
        install_mod(p, &path, 1);
        fflush(stdout);
    }
    return 0;
}
