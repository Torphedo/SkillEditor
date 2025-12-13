#include "skill_pack.hxx"
#include <common/logging.h>
#include "../mods.hxx"

bool test_sp4_creation(std::vector<std::string> paths, const char* expected_output_path) {
    const char* outpath = "_output.sp4";
    save_skill_pack(outpath, paths);

    const u32 output_size = file_size(outpath);
    const u32 expected_output_size = file_size(expected_output_path);
    if (output_size != expected_output_size) {
        LOG_MSG(error, "Output size is wrong (got %d bytes, expected %d)!\n", output_size, expected_output_size);
        return false;
    }

    u8* output = file_load(outpath);
    u8* expected_output = file_load(expected_output_path);

    if (!output || !expected_output) {
        free(output);
        free(expected_output);
        LOG_MSG(error, "Failed to load an output file!\n");
        return false;
    }

    const bool result = memcmp(expected_output, output, output_size) == 0;
    return result;
}
