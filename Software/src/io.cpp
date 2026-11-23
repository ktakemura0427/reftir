#include "rftir/io.hpp"

#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace rftir {

namespace {

// Relative to the executable's folder, the current folder, or RFTIR_SOURCE_DIR.
const char *const kDefaultLutPath = "LUT/fast_lut.bin";

bool isImageFile(const fs::path &path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return std::tolower(ch); });
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tif" || ext == ".tiff";
}

bool endsWith(const std::string &s, const std::string &suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

}  // namespace

std::vector<fs::path> listImages(const fs::path &dir, const std::string &suffix) {
    std::vector<fs::path> files;
    for (const auto &entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file() || !isImageFile(entry.path())) continue;
        if (!suffix.empty() && !endsWith(entry.path().stem().string(), suffix)) continue;
        files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    return files;
}

fs::path defaultOutputDir(const fs::path &input_dir) {
    fs::path input = fs::absolute(input_dir).lexically_normal();
    if (input.filename().empty()) input = input.parent_path();  // drop a trailing '/'
    return input.parent_path() / (input.filename().string() + "_recon");
}

std::string findDefaultLut(const char *argv0) {
    std::vector<fs::path> candidates;
    std::error_code ec;
    const fs::path exe_dir = fs::absolute(fs::path(argv0), ec).parent_path();
    if (!ec) candidates.push_back(exe_dir / kDefaultLutPath);
    candidates.push_back(fs::path(kDefaultLutPath));
#ifdef RFTIR_SOURCE_DIR
    candidates.push_back(fs::path(RFTIR_SOURCE_DIR) / kDefaultLutPath);
#endif
    for (const auto &path : candidates) {
        if (fs::exists(path, ec)) return path.string();
    }
    return kDefaultLutPath;
}

}  // namespace rftir
