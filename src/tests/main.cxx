#include "skill_pack.hxx"
#include <common/logging.h>

int main() {
    std::vector<std::string> pack_paths = {"mod_bundled_laser.sp4", "mod_ricochet.sp4"};
    bool result = test_sp4_creation(pack_paths, "pack_output.sp4");
    if (result) {
        LOG_MSG(info, "Passed pack creation test!\n");
    } else {
        LOG_MSG(error, "Failed pack creation test!\n");
    }

}