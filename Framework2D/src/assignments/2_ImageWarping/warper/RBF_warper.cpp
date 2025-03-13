// File: warper/rbf_warper.cpp
#include "RBF_warper.h"
#include <Eigen/Dense>
#include <cmath>
#include <algorithm>

namespace USTC_CG {
using namespace Eigen;

RBFWarper::RBFWarper(float mu) : mu_(mu), image_width_(0), image_height_(0) {}

void RBFWarper::setControlPoints(const std::vector<ControlPoint>& points) {
    p_.clear();
    q_.clear();
    for (const auto& cp : points) {
        p_.emplace_back(cp.src_x, cp.src_y);
        q_.emplace_back(cp.tar_x, cp.tar_y);
    }
    computeAffineTransform();
    computeRBFParameters();
}
// 添加方法实现
void RBFWarper::setImageSize(int width, int height) {
    image_width_ = width;
    image_height_ = height;
}

void RBFWarper::computeAffineTransform() {
    const size_t n = p_.size();
    A_ = Matrix2f::Identity();
    b_ = Vector2f::Zero();

    if (n >= 3) {
        // 最小二乘法求解仿射变换
        MatrixXf M(n, 3);
        for (size_t i = 0; i < n; ++i) {
            M.row(i) << p_[i].x(), p_[i].y(), 1.0f;
        }

        MatrixXf Q(n, 2);
        for (size_t i = 0; i < n; ++i) {
            Q.row(i) = q_[i].transpose();
        }

        MatrixXf params = M.bdcSvd(ComputeThinU | ComputeThinV).solve(Q);
        A_ << params(0, 0), params(1, 0),
              params(0, 1), params(1, 1);
        b_ << params(2, 0), params(2, 1);
    } else if (n == 1) {
        // 单点平移
        b_ = q_[0] - p_[0];
    } else if (n == 2) {
        // 两点缩放+平移
        Vector2f delta_p = p_[1] - p_[0];
        Vector2f delta_q = q_[1] - q_[0];
        float scale = delta_q.norm() / (delta_p.norm() + 1e-6f);
        A_ = Matrix2f::Identity() * scale;
        b_ = q_[0] - A_ * p_[0];
    }
}

void RBFWarper::computeRBFParameters() {
    const size_t n = p_.size();
    if (n == 0) return;

    // 计算径向基函数矩阵G
    G_.resize(n, n);
    for (size_t i = 0; i < n; ++i) {
        // 计算r_i为到最近邻的距离，最小1e-3防止除零
        float r_i = std::numeric_limits<float>::max();
        for (size_t j = 0; j < n; ++j) {
            if (i == j) continue;
            float dist = (p_[i] - p_[j]).norm();
            r_i = std::min(r_i, dist);
        }
        r_i = std::max(r_i, 1e-3f);

        for (size_t j = 0; j < n; ++j) {
            float d = (p_[i] - p_[j]).norm();
            G_(i, j) = std::pow(d * d + r_i * r_i, mu_ / 2.0f);
        }
    }

    // 构建约束矩阵
    MatrixXf constraints(3, n);
    for (size_t i = 0; i < n; ++i) {
        constraints(0, i) = p_[i].x();
        constraints(1, i) = p_[i].y();
        constraints(2, i) = 1.0f;
    }

    // 扩展矩阵并求解
    MatrixXf extended_G(n + 3, n + 3);
    extended_G.block(0, 0, n, n) = G_;
    extended_G.block(n, 0, 3, n) = constraints;
    extended_G.block(0, n, n, 3) = constraints.transpose();
    extended_G.block(n, n, 3, 3) = Matrix3f::Zero();

    MatrixXf rhs(n + 3, 2);
    for (size_t i = 0; i < n; ++i) {
        rhs.row(i) = (q_[i] - (A_ * p_[i] + b_)).transpose();
    }
    rhs.block(n, 0, 3, 2) = MatrixXf::Zero(3, 2);

    // 正则化SVD求解
    MatrixXf solution = extended_G.bdcSvd(ComputeThinU | ComputeThinV)
                        .solve(rhs + 1e-6 * MatrixXf::Identity(rhs.rows(), rhs.cols()));

    alpha_.resize(n);
    for (size_t i = 0; i < n; ++i) {
        alpha_[i] = solution.row(i).transpose();
    }
}

std::pair<float, float> RBFWarper::warp(float x, float y) const {
    if (p_.empty() || image_width_ <= 0 || image_height_ <= 0) 
        return {x, y};

    Vector2f p(x, y);
    Vector2f result = A_ * p + b_;

    for (size_t i = 0; i < p_.size(); ++i) {
        // 计算r_i（预计算优化）
        float r_i = std::numeric_limits<float>::max();
        for (size_t j = 0; j < p_.size(); ++j) {
            if (i == j) continue;
            float dist = (p_[i] - p_[j]).norm();
            r_i = std::min(r_i, dist);
        }
        r_i = std::max(r_i, 1e-3f);

        float d = (p - p_[i]).norm();
        float g = std::pow(d * d + r_i * r_i, mu_ / 2.0f);
        result += alpha_[i] * g;
    }

    // 坐标边界约束
    result.x() = std::clamp(result.x(), 0.0f, static_cast<float>(image_width_ - 1));
    result.y() = std::clamp(result.y(), 0.0f, static_cast<float>(image_height_ - 1));

    return {result.x(), result.y()};
} // Added the missing closing brace here
} // namespace USTC_CG