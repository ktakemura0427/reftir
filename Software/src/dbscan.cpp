#include "rftir/dbscan.hpp"

#include <opencv2/imgproc.hpp>

#include <cmath>

namespace rftir {

namespace {

constexpr int kUnvisited = 0;
constexpr int kNoise = -1;

// Answers radius queries on integer points through a dense lookup table over their bounding box.
class GridNeighborSearch {
public:
    GridNeighborSearch(const std::vector<cv::Point> &points, float eps)
        : points_(points), bbox_(cv::boundingRect(points)),
          index_map_(static_cast<size_t>(bbox_.width) * bbox_.height, -1) {
        for (int i = 0; i < static_cast<int>(points_.size()); ++i) index_map_[mapIndex(points_[i])] = i;

        const int reach = static_cast<int>(std::floor(eps));
        for (int dx = -reach; dx <= reach; ++dx)
            for (int dy = -reach; dy <= reach; ++dy)
                if (static_cast<float>(dx * dx + dy * dy) <= eps * eps) offsets_.emplace_back(dx, dy);
    }

    void query(int i, std::vector<int> &neighbors) const {
        neighbors.clear();
        for (const auto &offset : offsets_) {
            const cv::Point p = points_[i] + offset;
            if (!bbox_.contains(p)) continue;
            const int j = index_map_[mapIndex(p)];
            if (j >= 0) neighbors.push_back(j);
        }
    }

private:
    size_t mapIndex(const cv::Point &p) const {
        return static_cast<size_t>(p.y - bbox_.y) * bbox_.width + (p.x - bbox_.x);
    }

    const std::vector<cv::Point> &points_;
    cv::Rect bbox_;
    std::vector<int> index_map_;
    std::vector<cv::Point> offsets_;
};

}  // namespace

int clusterDbscan(const std::vector<cv::Point> &points, const DbscanParams &params, std::vector<int> &labels) {
    const int n = static_cast<int>(points.size());
    labels.assign(n, kUnvisited);
    if (n == 0) return 0;

    const GridNeighborSearch search(points, params.eps);
    std::vector<int> neighbors, frontier;
    int cluster_id = 0;

    for (int i = 0; i < n; ++i) {
        if (labels[i] != kUnvisited) continue;
        search.query(i, neighbors);
        if (static_cast<int>(neighbors.size()) < params.min_pts) {
            labels[i] = kNoise;  // may become a border point of a later cluster
            continue;
        }

        labels[i] = ++cluster_id;
        frontier.clear();
        auto absorbNeighbors = [&]() {
            for (int j : neighbors) {
                if (labels[j] == kNoise) {
                    labels[j] = cluster_id;
                } else if (labels[j] == kUnvisited) {
                    labels[j] = cluster_id;
                    frontier.push_back(j);
                }
            }
        };
        absorbNeighbors();
        for (size_t k = 0; k < frontier.size(); ++k) {
            search.query(frontier[k], neighbors);
            if (static_cast<int>(neighbors.size()) >= params.min_pts) absorbNeighbors();
        }
    }
    return cluster_id;
}

std::vector<Cluster> summarizeClusters(const std::vector<cv::Point> &points, const std::vector<int> &labels,
                                       int num_clusters) {
    std::vector<Cluster> clusters(num_clusters);
    std::vector<cv::Point2d> sums(num_clusters, cv::Point2d(0.0, 0.0));
    for (int k = 0; k < num_clusters; ++k) clusters[k] = {k + 1, cv::Point2f(0.0f, 0.0f), 0};
    for (size_t i = 0; i < points.size(); ++i) {
        if (labels[i] <= 0) continue;
        const int k = labels[i] - 1;
        sums[k] += cv::Point2d(points[i].x, points[i].y);
        clusters[k].num_points++;
    }
    for (int k = 0; k < num_clusters; ++k) {
        if (clusters[k].num_points == 0) continue;
        clusters[k].centroid = cv::Point2f(static_cast<float>(sums[k].x / clusters[k].num_points),
                                           static_cast<float>(sums[k].y / clusters[k].num_points));
    }
    return clusters;
}

}  // namespace rftir
