// Post-processing of differential RFTIR images from a single device:
//   1. reconstruct binary contact images on the contact plane with a BIN LUT, and
//   2. cluster the contact grid points with DBSCAN to remove noise and locate touches.
// See README.md for usage and output files.

#include "rftir/dbscan.hpp"
#include "rftir/io.hpp"
#include "rftir/lut_reconstructor.hpp"
#include "rftir/options.hpp"
#include "rftir/visualization.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct FrameResult {
    cv::Mat recon;
    cv::Mat denoised;
    cv::Mat cluster_image;
    std::vector<rftir::Cluster> clusters;
    int contact_px = 0;
    int num_noise = 0;
};

FrameResult processFrame(const cv::Mat &diff, const rftir::LutReconstructor &reconstructor,
                         const rftir::Options &options) {
    FrameResult result;
    std::vector<cv::Point> contact_points;
    result.recon = reconstructor.reconstruct(diff, options.recon, &contact_points);
    result.contact_px = cv::countNonZero(result.recon);

    std::vector<int> labels;
    const int num_clusters = rftir::clusterDbscan(contact_points, options.dbscan, labels);
    result.clusters = rftir::summarizeClusters(contact_points, labels, num_clusters);
    result.num_noise = static_cast<int>(std::count(labels.begin(), labels.end(), -1));
    result.denoised = rftir::renderDenoised(contact_points, labels, options.recon);
    result.cluster_image = rftir::renderClusters(contact_points, labels, result.clusters, options.recon);
    return result;
}

void printFrameSummary(const std::string &input_name, const std::string &save_name, const FrameResult &result) {
    std::cout << "  " << input_name << " -> " << save_name << " (contact pixels: " << result.contact_px
              << ", clusters: " << result.clusters.size() << ", noise points: " << result.num_noise << ")" << std::endl;
    for (const auto &cluster : result.clusters) {
        std::cout << "      #" << cluster.id << " centroid (X, Z) = (" << std::fixed << std::setprecision(1)
                  << cluster.centroid.x << ", " << cluster.centroid.y << ") mm, " << cluster.num_points << " points"
                  << std::defaultfloat << std::endl;
    }
}

}  // namespace

int main(int argc, char const *argv[]) {
    rftir::Options options;
    switch (rftir::parseOptions(argc, argv, options)) {
        case rftir::ParseStatus::kHelp: return 0;
        case rftir::ParseStatus::kError: return 1;
        case rftir::ParseStatus::kOk: break;
    }

    const fs::path input_dir(options.input_dir);
    if (!fs::is_directory(input_dir)) {
        std::cerr << "Error: input folder does not exist: " << options.input_dir << std::endl;
        return 1;
    }
    const fs::path output_dir =
        options.output_dir.empty() ? rftir::defaultOutputDir(input_dir) : fs::path(options.output_dir);
    fs::create_directories(output_dir);
    if (fs::equivalent(input_dir, output_dir)) {
        std::cerr << "Error: the output folder must differ from the input folder (input images would be overwritten)."
                  << std::endl;
        return 1;
    }
    const std::string lut_path = options.lut_path.empty() ? rftir::findDefaultLut(argv[0]) : options.lut_path;

    const rftir::ReconstructionParams &recon = options.recon;
    std::cout << "Input      : " << input_dir.string() << std::endl;
    std::cout << "Output     : " << output_dir.string() << std::endl;
    std::cout << "LUT        : " << lut_path << std::endl;
    std::cout << "Output size: " << recon.rows << "x" << recon.cols << ", pixel_size=" << recon.pixel_size
              << ", match_ratio=" << recon.match_ratio << ", diff_thresh=" << recon.diff_thresh << std::endl;
    std::cout << "DBSCAN     : eps=" << options.dbscan.eps << " mm, min_pts=" << options.dbscan.min_pts << std::endl;

    const fs::path denoised_dir = output_dir / "denoised";
    const fs::path cluster_dir = output_dir / "clusters";
    fs::create_directories(denoised_dir);
    fs::create_directories(cluster_dir);

    rftir::LutReconstructor reconstructor;
    if (!reconstructor.load(lut_path)) return 1;
    const rftir::LutGeometry &geometry = reconstructor.geometry();

    const std::vector<fs::path> files = rftir::listImages(input_dir, options.suffix);
    if (files.empty()) {
        std::cerr << "Error: no images to process in " << input_dir.string() << std::endl;
        return 1;
    }
    std::cout << "Processing " << files.size() << " images" << std::endl;

    int num_saved = 0, num_failed = 0, num_touched = 0;
    for (const auto &file : files) {
        const std::string input_name = file.filename().string();
        const cv::Mat diff = cv::imread(file.string(), cv::IMREAD_GRAYSCALE);
        if (diff.empty()) {
            std::cerr << "Warning: failed to read " << input_name << ", skipped" << std::endl;
            num_failed++;
            continue;
        }
        if (diff.cols != geometry.image_width || diff.rows != geometry.image_height) {
            std::cerr << "Warning: " << input_name << " is " << diff.cols << "x" << diff.rows << ", but the LUT expects "
                      << geometry.image_width << "x" << geometry.image_height << " (pixels outside the LUT are ignored)"
                      << std::endl;
        }

        const FrameResult result = processFrame(diff, reconstructor, options);
        if (result.contact_px > 0) num_touched++;

        const std::string save_name = file.stem().string() + ".png";  // always lossless PNG
        if (!cv::imwrite((output_dir / save_name).string(), result.recon) ||
            !cv::imwrite((denoised_dir / save_name).string(), result.denoised) ||
            !cv::imwrite((cluster_dir / save_name).string(), result.cluster_image)) {
            std::cerr << "Warning: failed to save " << save_name << std::endl;
            num_failed++;
            continue;
        }
        num_saved++;
        printFrameSummary(input_name, save_name, result);
    }

    std::cout << "Done: saved " << num_saved << " images (" << num_touched << " with contact), failed " << num_failed
              << std::endl;
    return num_failed == 0 ? 0 : 1;
}
