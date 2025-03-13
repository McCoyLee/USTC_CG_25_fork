#pragma once
#include "warper.h"
#include <dlib/dnn.h>
#include <dlib/matrix.h>

namespace USTC_CG {
using namespace dlib;

using net_type = loss_mean_squared_multioutput<
    fc<2,
    relu<fc<10,
    relu<fc<10,
    input<matrix<float>>
    >>>>>>;

class NNWarper : public Warper {
public:
    NNWarper();
    virtual ~NNWarper() = default;

    void setControlPoints(const std::vector<ControlPoint>& points) override;
    std::pair<float, float> warp(float x, float y) const override; // 严格匹配基类

    void setImageSize(int width, int height);

private:
    void trainNetwork();
    mutable net_type net_;
    std::vector<matrix<float>> inputs_;
    std::vector<matrix<float>> targets_;
    int image_width_ = 0;
    int image_height_ = 0;
};
} // namespace USTC_CG