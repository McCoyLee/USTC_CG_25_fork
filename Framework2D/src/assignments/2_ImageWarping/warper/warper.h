#pragma once
#include <vector>
#include <utility> // for std::pair

namespace USTC_CG
{
// 控制点结构体（起点+终点）
struct ControlPoint {
    float src_x, src_y;
    float tar_x, tar_y;
    ControlPoint(float sx, float sy, float tx, float ty) 
        : src_x(sx), src_y(sy), tar_x(tx), tar_y(ty) {}
};

class Warper {
public:
    virtual ~Warper() = default;
    
    // 初始化控制点（必须在warp前调用）
    virtual void setControlPoints(const std::vector<ControlPoint>& points) = 0;
    
    // 核心接口：输入源图像坐标(x,y)，输出变形后的坐标(fx,fy)
    virtual std::pair<float, float> warp(float x, float y) const = 0;
};
} // namespace USTC_CG