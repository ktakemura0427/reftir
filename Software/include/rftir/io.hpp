#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace rftir {

// Image files directly under dir (png/jpg/jpeg/bmp/tif/tiff), sorted by path.
// If suffix is not empty, only files whose stem ends with it are returned.
std::vector<std::filesystem::path> listImages(const std::filesystem::path &dir, const std::string &suffix);

// <input>_recon next to the input folder.
std::filesystem::path defaultOutputDir(const std::filesystem::path &input_dir);

// Default LUT (LUT/...) under the executable's folder, the current folder, or RFTIR_SOURCE_DIR (set by CMake).
std::string findDefaultLut(const char *argv0);

}  // namespace rftir
