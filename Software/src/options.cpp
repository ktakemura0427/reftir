#include "rftir/options.hpp"

#include <iostream>
#include <stdexcept>

namespace rftir {

void printUsage() {
    std::cout << "RFTIR contact-plane reconstruction (single device, post-processing)\n\n"
              << "Usage:\n"
              << "  ./build/Singledevice_postprocess --input <dir> [options]\n\n"
              << "Options:\n"
              << "  --input, -i <dir>      Folder of differential binary RFTIR images (required)\n"
              << "  --output, -o <dir>     Output folder (default: <input>_recon)\n"
              << "  --lut, -l <path>       BIN LUT (default: LUT/fast_lut.bin under the executable's folder,\n"
              << "                         the current folder, or the source folder)\n"
              << "  --suffix <str>         Process only images whose name (without extension) ends with this (e.g. _diff)\n"
              << "  --rows <int>           Rows of the output images (default: 320)\n"
              << "  --cols <int>           Columns of the output images (default: 640)\n"
              << "  --pixel-size <int>     Drawing size of a contact grid point [px] (default: 1)\n"
              << "  --match-ratio <float>  Match ratio required for contact (default: 1.0 = exact match)\n"
              << "  --diff-thresh <int>    Pixels above this value are treated as lit (default: 0 = any non-zero pixel)\n"
              << "  --eps <float>          DBSCAN neighborhood radius [mm] (default: 6)\n"
              << "  --min-pts <int>        DBSCAN neighbors required for a core point, including itself (default: 42)\n"
              << "  --help, -h             Show this help\n"
              << "\n"
              << "Output:\n"
              << "  <output>/<name>.png           Binary reconstruction image\n"
              << "  <output>/denoised/<name>.png  Binary reconstruction image without DBSCAN noise points\n"
              << "  <output>/clusters/<name>.png  Cluster visualization (colors, centroids, cluster IDs)\n"
              << std::endl;
}

ParseStatus parseOptions(int argc, const char *argv[], Options &options) {
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            const bool has_value = i + 1 < argc;
            if ((arg == "--input" || arg == "-i") && has_value) {
                options.input_dir = argv[++i];
            } else if ((arg == "--output" || arg == "-o") && has_value) {
                options.output_dir = argv[++i];
            } else if ((arg == "--lut" || arg == "-l") && has_value) {
                options.lut_path = argv[++i];
            } else if (arg == "--suffix" && has_value) {
                options.suffix = argv[++i];
            } else if (arg == "--rows" && has_value) {
                options.recon.rows = std::stoi(argv[++i]);
            } else if (arg == "--cols" && has_value) {
                options.recon.cols = std::stoi(argv[++i]);
            } else if (arg == "--pixel-size" && has_value) {
                options.recon.pixel_size = std::stoi(argv[++i]);
            } else if (arg == "--match-ratio" && has_value) {
                options.recon.match_ratio = std::stof(argv[++i]);
            } else if (arg == "--diff-thresh" && has_value) {
                options.recon.diff_thresh = std::stoi(argv[++i]);
            } else if (arg == "--eps" && has_value) {
                options.dbscan.eps = std::stof(argv[++i]);
            } else if (arg == "--min-pts" && has_value) {
                options.dbscan.min_pts = std::stoi(argv[++i]);
            } else if (arg == "--help" || arg == "-h") {
                printUsage();
                return ParseStatus::kHelp;
            } else {
                std::cerr << "Error: unknown option or missing value: " << arg << std::endl;
                printUsage();
                return ParseStatus::kError;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: invalid numeric value (" << e.what() << ")" << std::endl;
        return ParseStatus::kError;
    }

    const ReconstructionParams &recon = options.recon;
    if (options.input_dir.empty()) {
        std::cerr << "Error: --input is required." << std::endl;
        printUsage();
        return ParseStatus::kError;
    }
    if (recon.rows <= 0 || recon.cols <= 0 || recon.pixel_size <= 0 || recon.match_ratio <= 0.0f ||
        recon.match_ratio > 1.0f) {
        std::cerr << "Error: --rows, --cols, and --pixel-size must be positive, and --match-ratio must be in (0, 1]."
                  << std::endl;
        return ParseStatus::kError;
    }
    if (options.dbscan.eps <= 0.0f || options.dbscan.min_pts < 1) {
        std::cerr << "Error: --eps must be positive, and --min-pts must be at least 1." << std::endl;
        return ParseStatus::kError;
    }
    return ParseStatus::kOk;
}

}  // namespace rftir
