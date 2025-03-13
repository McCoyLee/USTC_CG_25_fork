#include "nn_warper.h"
#include <dlib/optimization.h>

namespace USTC_CG {
using namespace dlib;

NNWarper::NNWarper() {
    net_ = net_type();
}

void NNWarper::setImageSize(int width, int height) {
    image_width_ = width;
    image_height_ = height;
}

void NNWarper::setControlPoints(const std::vector<ControlPoint>& points) {
    inputs_.clear();
    targets_.clear();

    for (const auto& cp : points) {
        matrix<float> input(2, 1);
        input(0) = cp.src_x / static_cast<float>(image_width_);
        input(1) = cp.src_y / static_cast<float>(image_height_);
        inputs_.push_back(input);

        matrix<float> target(2, 1);
        target(0) = cp.tar_x / static_cast<float>(image_width_);
        target(1) = cp.tar_y / static_cast<float>(image_height_);
        targets_.push_back(target);
    }

    trainNetwork();
}

void NNWarper::trainNetwork() {
    dnn_trainer<net_type, adam> trainer(net_);
    trainer.set_learning_rate(0.001);
    trainer.set_mini_batch_size(16);
    trainer.set_max_num_epochs(2000);
    trainer.train(inputs_, targets_);
}

std::pair<float, float> NNWarper::warp(float x, float y) const {
    matrix<float> input(1, 2);
    input(0, 0) = x / image_width_;   // 假设归一化处理
    input(0, 1) = y / image_height_;
    
    // 由于 net_ 是 mutable，可以在 const 函数中调用
    matrix<float> output = net_(input); 
    
    return { output(0) * image_width_, output(1) * image_height_ };
}
} // namespace USTC_CG