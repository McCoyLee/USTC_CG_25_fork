#include "idw_warper.h"
#include <Eigen/Dense>
#include <cmath>

namespace USTC_CG
{
using namespace Eigen;

void IDWWarper::setControlPoints(const std::vector<ControlPoint>& points) {
    control_points_ = points;
    precomputeTransformMatrices();
}

void IDWWarper::precomputeTransformMatrices() {
    const size_t n = control_points_.size();
    T_matrices_.resize(n);

    for (size_t i = 0; i < n; ++i) {
        const auto& pi = control_points_[i];
        Matrix2f A = Matrix2f::Zero();
        Matrix2f B = Matrix2f::Zero();

        // 计算矩阵A和B
        for (size_t j = 0; j < n; ++j) {
            if (i == j) continue;
            const auto& pj = control_points_[j];
            
            Vector2f delta(pj.src_x - pi.src_x, pj.src_y - pi.src_y);
            float dist_sq = delta.squaredNorm();
            if (dist_sq < 1e-6f) continue;
            
            float sigma = 1.0f / dist_sq; // μ=2时的IDW参数
            A += sigma * delta * delta.transpose();
            
            Vector2f q_delta(pj.tar_x - pi.tar_x, pj.tar_y - pi.tar_y);
            B += sigma * q_delta * delta.transpose();
        }

        // 求A的逆并计算T矩阵
        float det = A.determinant();
        if (std::abs(det) < 1e-6f) {
            T_matrices_[i] = Matrix2f::Identity();
        } else {
            T_matrices_[i] = B * A.inverse();
        }
    }
}

std::pair<float, float> IDWWarper::warp(float x, float y) const {
    std::vector<float> weights(control_points_.size(), 0.0f);
    float total_weight = 0.0f;
    bool exact_match = false;
    size_t exact_idx = 0;

    // 计算权重
    for (size_t i = 0; i < control_points_.size(); ++i) {
        const auto& p = control_points_[i];
        float dx = x - p.src_x;
        float dy = y - p.src_y;
        float dist_sq = dx*dx + dy*dy;

        if (dist_sq < 1e-6f) {
            exact_match = true;
            exact_idx = i;
            break;
        }
        weights[i] = 1.0f / dist_sq;
        total_weight += weights[i];
    }

    // 处理精确匹配情况
    if (exact_match) {
        const auto& p = control_points_[exact_idx];
        return {p.tar_x, p.tar_y};
    }

    // 计算加权平均
    Vector2f result(0.0f, 0.0f);
    for (size_t i = 0; i < control_points_.size(); ++i) {
        const auto& p = control_points_[i];
        float weight = weights[i] / total_weight;
        
        Vector2f delta(x - p.src_x, y - p.src_y);
        Vector2f t = T_matrices_[i] * delta;
        result += weight * (Vector2f(p.tar_x, p.tar_y) + t);
    }

    return {result.x(), result.y()};
}
} // namespace USTC_CG