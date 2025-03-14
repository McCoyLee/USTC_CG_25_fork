#pragma once
#include "warper.h"
#include <Eigen/Dense>
#include <vector>

namespace USTC_CG
{
class IDWWarper : public Warper {
public:
    IDWWarper() = default;
    virtual ~IDWWarper() = default;
    
    void setControlPoints(const std::vector<ControlPoint>& points) override;
    std::pair<float, float> warp(float x, float y) const override;

private:
    std::vector<ControlPoint> control_points_;
    std::vector<Eigen::Matrix2f> T_matrices_; // Ԥ����ı任����
    
    void precomputeTransformMatrices(); // ������ѧ�߼�
};
} // namespace USTC_CG