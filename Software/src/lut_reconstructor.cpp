#include "rftir/lut_reconstructor.hpp"

#include "rftir/grid_image.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <unordered_set>

namespace rftir {

namespace {

// On-disk record written by GenerateFastLUT.cpp.
#pragma pack(push, 1)
struct ReflectionPoint {
    int32_t pixel_x;
    int32_t pixel_y;
    int32_t bounce_id;
    float world_x;  // [mm]
    float world_z;  // [mm]
};
#pragma pack(pop)
static_assert(sizeof(ReflectionPoint) == 20, "ReflectionPoint must be 20 bytes");

struct PointHash {
    size_t operator()(const cv::Point &p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

bool readRecords(const std::string &path, std::vector<ReflectionPoint> &records) {
    FILE *fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
        std::cerr << "Error: cannot open the LUT file: " << path << std::endl;
        return false;
    }
    uint64_t count = 0;
    if (std::fread(&count, sizeof(count), 1, fp) != 1) {
        std::cerr << "Error: failed to read the LUT header: " << path << std::endl;
        std::fclose(fp);
        return false;
    }
    records.resize(count);
    const size_t read_count = std::fread(records.data(), sizeof(ReflectionPoint), count, fp);
    std::fclose(fp);
    if (read_count != count) {
        std::cerr << "Error: the LUT file is truncated (expected " << count << " records, read " << read_count << ")"
                  << std::endl;
        return false;
    }
    std::cout << "Loaded " << count << " LUT records" << std::endl;
    return true;
}

}  // namespace

LutReconstructor::LutReconstructor(const LutGeometry &geometry) : geometry_(geometry) {}

size_t LutReconstructor::gridIndex(int x, int z) const {
    return static_cast<size_t>(x - geometry_.min_x) * geometry_.numZ() + static_cast<size_t>(z - geometry_.min_z);
}

cv::Point LutReconstructor::gridPoint(size_t index) const {
    const int num_z = geometry_.numZ();
    return cv::Point(geometry_.min_x + static_cast<int>(index / num_z), geometry_.min_z + static_cast<int>(index % num_z));
}

bool LutReconstructor::load(const std::string &bin_path) {
    std::vector<ReflectionPoint> records;
    if (!readRecords(bin_path, records)) return false;

    const LutGeometry &g = geometry_;
    const float tol = g.grid_tolerance;
    std::vector<std::unordered_set<cv::Point, PointHash>> grid_pixels(g.numGrids());
    for (const auto &rec : records) {
        if (rec.pixel_x < 0 || rec.pixel_x >= g.image_width || rec.pixel_y < 0 || rec.pixel_y >= g.image_height) continue;

        const int x_begin = std::max(g.min_x, static_cast<int>(std::round(rec.world_x - tol)));
        const int x_end = std::min(g.max_x, static_cast<int>(std::round(rec.world_x + tol)));
        const int z_begin = std::max(g.min_z, static_cast<int>(std::round(rec.world_z - tol)));
        const int z_end = std::min(g.max_z, static_cast<int>(std::round(rec.world_z + tol)));
        for (int x = x_begin; x <= x_end; ++x) {
            if (std::abs(rec.world_x - static_cast<float>(x)) > tol) continue;
            for (int z = z_begin; z <= z_end; ++z) {
                if (std::abs(rec.world_z - static_cast<float>(z)) > tol) continue;
                grid_pixels[gridIndex(x, z)].insert(cv::Point(rec.pixel_x, rec.pixel_y));
            }
        }
    }

    entry_sizes_.assign(g.numGrids(), 0);
    reverse_index_.assign(static_cast<size_t>(g.image_height) * g.image_width, {});
    size_t active_grids = 0;
    for (size_t i = 0; i < g.numGrids(); ++i) {
        entry_sizes_[i] = grid_pixels[i].size();
        if (entry_sizes_[i] > 0) active_grids++;
        for (const auto &pt : grid_pixels[i]) {
            reverse_index_[static_cast<size_t>(pt.y) * g.image_width + pt.x].push_back(i);
        }
    }
    std::cout << "Built the LUT index: " << active_grids << " / " << g.numGrids() << " grid points are active"
              << std::endl;
    return true;
}

cv::Mat LutReconstructor::reconstruct(const cv::Mat &diff_img, const ReconstructionParams &params,
                                      std::vector<cv::Point> *contact_points) const {
    cv::Mat recon = cv::Mat::zeros(params.rows, params.cols, CV_8UC1);
    if (contact_points) contact_points->clear();
    if (diff_img.empty()) return recon;

    cv::Mat gray = diff_img;
    if (gray.channels() > 1) cv::cvtColor(diff_img, gray, cv::COLOR_BGR2GRAY);
    std::vector<cv::Point> lit_pixels;
    cv::findNonZero(gray > params.diff_thresh, lit_pixels);
    if (lit_pixels.empty()) return recon;

    const LutGeometry &g = geometry_;
    std::vector<int> match_count(g.numGrids(), 0);
    for (const auto &pt : lit_pixels) {
        if (pt.x < 0 || pt.x >= g.image_width || pt.y < 0 || pt.y >= g.image_height) continue;
        for (size_t i : reverse_index_[static_cast<size_t>(pt.y) * g.image_width + pt.x]) match_count[i]++;
    }

    const cv::Rect image_rect(0, 0, params.cols, params.rows);
    for (size_t i = 0; i < g.numGrids(); ++i) {
        if (entry_sizes_[i] == 0) continue;
        const int required = std::max(1, static_cast<int>(std::ceil(entry_sizes_[i] * params.match_ratio)));
        if (match_count[i] < required) continue;

        const cv::Point grid = gridPoint(i);
        const cv::Point pixel = gridToPixel(grid, params.rows);
        if (!image_rect.contains(pixel)) continue;
        drawGridPoint<uchar>(recon, pixel, params.pixel_size, 255);
        if (contact_points) contact_points->push_back(grid);
    }
    return recon;
}

}  // namespace rftir
