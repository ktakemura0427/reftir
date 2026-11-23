#pragma once

#include "rftir/dbscan.hpp"
#include "rftir/lut_reconstructor.hpp"

#include <opencv2/core.hpp>

#include <vector>

namespace rftir {

// Binary image (CV_8UC1, 0 / 255) of the points that belong to a cluster.
cv::Mat renderDenoised(const std::vector<cv::Point> &points, const std::vector<int> &labels,
                       const ReconstructionParams &params);

// Color image (CV_8UC3): one color per cluster, noise in gray, centroids as red dots with ids.
cv::Mat renderClusters(const std::vector<cv::Point> &points, const std::vector<int> &labels,
                       const std::vector<Cluster> &clusters, const ReconstructionParams &params);

}  // namespace rftir
