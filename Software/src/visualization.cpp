#include "rftir/visualization.hpp"

#include "rftir/grid_image.hpp"

#include <opencv2/imgproc.hpp>

#include <string>

namespace rftir {

namespace {

const cv::Vec3b kPalette[] = {
    {255, 128, 0}, {0, 255, 0}, {255, 0, 255}, {0, 255, 255},
    {255, 255, 0}, {0, 160, 255}, {255, 0, 128}, {128, 255, 128},
};
constexpr int kPaletteSize = static_cast<int>(sizeof(kPalette) / sizeof(kPalette[0]));
const cv::Vec3b kNoiseColor(110, 110, 110);

}  // namespace

cv::Mat renderDenoised(const std::vector<cv::Point> &points, const std::vector<int> &labels,
                       const ReconstructionParams &params) {
    cv::Mat img = cv::Mat::zeros(params.rows, params.cols, CV_8UC1);
    for (size_t i = 0; i < points.size(); ++i) {
        if (labels[i] <= 0) continue;
        drawGridPoint<uchar>(img, gridToPixel(points[i], params.rows), params.pixel_size, 255);
    }
    return img;
}

cv::Mat renderClusters(const std::vector<cv::Point> &points, const std::vector<int> &labels,
                       const std::vector<Cluster> &clusters, const ReconstructionParams &params) {
    cv::Mat img = cv::Mat::zeros(params.rows, params.cols, CV_8UC3);
    for (size_t i = 0; i < points.size(); ++i) {
        const cv::Vec3b &color = labels[i] > 0 ? kPalette[(labels[i] - 1) % kPaletteSize] : kNoiseColor;
        drawGridPoint<cv::Vec3b>(img, gridToPixel(points[i], params.rows), params.pixel_size, color);
    }
    for (const auto &cluster : clusters) {
        const cv::Point2f center_f = gridToPixel(cluster.centroid, params.rows);
        const cv::Point center(cvRound(center_f.x), cvRound(center_f.y));
        cv::circle(img, center, 3, cv::Scalar(0, 0, 255), cv::FILLED, cv::LINE_AA);
        cv::circle(img, center, 4, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
        cv::putText(img, std::to_string(cluster.id), center + cv::Point(6, -6), cv::FONT_HERSHEY_SIMPLEX, 0.4,
                    cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    }
    return img;
}

}  // namespace rftir
