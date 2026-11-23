#pragma once

#include <opencv2/core.hpp>

#include <vector>

namespace rftir {

// Defaults tuned for finger touches reconstructed with a 1 mm grid (see README).
struct DbscanParams {
    float eps = 6.0f;  // neighborhood radius [mm]
    int min_pts = 42;  // neighbors (including the point itself) required for a core point
};

struct Cluster {
    int id;                // 1-based
    cv::Point2f centroid;  // (x, z) [mm]
    int num_points;
};

// Exact DBSCAN on integer grid points (neighbors: distance <= eps). Points are visited in the
// given order, so the result is deterministic. labels[i] is -1 for noise or the 1-based cluster id.
// Returns the number of clusters.
int clusterDbscan(const std::vector<cv::Point> &points, const DbscanParams &params, std::vector<int> &labels);

// Centroid and size of each cluster, ordered by cluster id.
std::vector<Cluster> summarizeClusters(const std::vector<cv::Point> &points, const std::vector<int> &labels,
                                       int num_clusters);

}  // namespace rftir
