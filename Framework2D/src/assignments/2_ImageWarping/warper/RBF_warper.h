// File: warper/rbf_warper.h
#pragma once
#include "warper.h"
#include <Eigen/Dense>
#include <vector>

namespace USTC_CG {
class RBFWarper : public Warper {
public:
    explicit RBFWarper(float mu = 1.0f);
    virtual ~RBFWarper() = default;

    void setImageSize(int width, int height);

    void setControlPoints(const std::vector<ControlPoint>& points) override;
    std::pair<float, float> warp(float x, float y) const override;

private:
    void computeAffineTransform();    // 计算仿射变换参数
    void computeRBFParameters();        // 计算RBF系数
    float mu_;                        // 径向基函数指数
    Eigen::Matrix2f A_;                // 仿射变换矩阵
    Eigen::Vector2f b_;                // 仿射平移向量
    std::vector<Eigen::Vector2f> alpha_; // RBF系数
    std::vector<Eigen::Vector2f> p_;   // 源控制点坐标
    std::vector<Eigen::Vector2f> q_;   // 目标控制点坐标
    Eigen::MatrixXf G_;                // 径向基函数矩阵
    int image_width_;                  // 图像宽度
    int image_height_;                 // 图像高度
};
} // namespace USTC_CG