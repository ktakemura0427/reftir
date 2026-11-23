#pragma once

#include <opencv2/core.hpp>

namespace rftir {

// Output image layout (1 px = 1 mm): row = rows / 2 - x, col = z.
inline cv::Point gridToPixel(const cv::Point &grid, int rows) {
    return cv::Point(grid.y, rows / 2 - grid.x);
}

inline cv::Point2f gridToPixel(const cv::Point2f &grid, int rows) {
    return cv::Point2f(grid.y, static_cast<float>(rows / 2) - grid.x);
}

// Draws one grid point as a pixel_size x pixel_size block. T is uchar (1ch) or cv::Vec3b (3ch).
template <typename T>
void drawGridPoint(cv::Mat &img, const cv::Point &pixel, int pixel_size, const T &value) {
    if (pixel_size <= 1) {
        img.at<T>(pixel.y, pixel.x) = value;
        return;
    }
    const int half = (pixel_size - 1) / 2;
    const int extra = (pixel_size % 2 == 0) ? 1 : 0;
    for (int dr = -half; dr <= half + extra; dr++) {
        for (int dc = -half; dc <= half + extra; dc++) {
            const int r = pixel.y + dr, c = pixel.x + dc;
            if (r >= 0 && r < img.rows && c >= 0 && c < img.cols) img.at<T>(r, c) = value;
        }
    }
}

}  // namespace rftir
