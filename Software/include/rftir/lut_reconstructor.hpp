#pragma once

#include <opencv2/core.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace rftir {

// Camera and grid geometry the BIN LUT was generated for.
struct LutGeometry {
    int image_width = 768;
    int image_height = 432;
    int min_x = -180;  // [mm]
    int max_x = 180;
    int min_z = 220;
    int max_z = 520;
    float grid_tolerance = 0.70f;  // reflection points within this distance [mm] belong to a grid point

    int numX() const { return max_x - min_x + 1; }
    int numZ() const { return max_z - min_z + 1; }
    size_t numGrids() const { return static_cast<size_t>(numX()) * numZ(); }
};

struct ReconstructionParams {
    int rows = 320;
    int cols = 640;
    int pixel_size = 1;       // drawing size of one grid point [px]
    float match_ratio = 1.0f; // 1.0 = every LUT pixel of a grid point must be lit
    int diff_thresh = 0;      // input pixels above this value are lit
};

// Reconstructs binary contact images on the contact plane from differential RFTIR images.
class LutReconstructor {
public:
    explicit LutReconstructor(const LutGeometry &geometry = LutGeometry());

    // Loads a BIN LUT: uint64 record count followed by 20-byte records.
    bool load(const std::string &bin_path);

    // Returns a CV_8UC1 image (contact = 255). contact_points, if given, receives the contact
    // grid points (x, z) [mm] that fall inside the image, in LUT grid order.
    cv::Mat reconstruct(const cv::Mat &diff_img, const ReconstructionParams &params,
                        std::vector<cv::Point> *contact_points = nullptr) const;

    const LutGeometry &geometry() const { return geometry_; }

private:
    size_t gridIndex(int x, int z) const;
    cv::Point gridPoint(size_t index) const;

    LutGeometry geometry_;
    std::vector<size_t> entry_sizes_;                 // number of camera pixels per grid point
    std::vector<std::vector<size_t>> reverse_index_;  // camera pixel (y * width + x) -> grid points
};

}  // namespace rftir
