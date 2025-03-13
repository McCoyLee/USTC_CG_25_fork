#pragma once
#include "common/image_widget.h"
#include "warper/warper.h"  // 包含Warper基类头文件

namespace USTC_CG
{
    using uchar = unsigned char;
class WarpingWidget : public ImageWidget {
public:
    explicit WarpingWidget(const std::string& label, const std::string& filename);
    virtual ~WarpingWidget() noexcept = default;

    void draw() override;

    // 编辑功能
    void invert();
    void mirror(bool is_horizontal, bool is_vertical);
    void gray_scale();
    void warping();
    void restore();

    // 变形类型枚举
    enum WarpingType {
        kDefault = 0,
        kFisheye = 1,
        kIDW = 2,
        kRBF = 3,
        kNeuralNetwork = 4,
    };
    void set_default();
    void set_fisheye();
    void set_IDW();
    void set_RBF();
    void set_NeuralNetwork();

    // 控制点交互
    void enable_selecting(bool flag);
    void select_points();
    void init_selections();

private:
    std::shared_ptr<Image> back_up_;  // 原始图像备份
    std::vector<ControlPoint> control_points_;  // 统一控制点存储（替换旧变量）
    std::unique_ptr<Warper> current_warper_;    // 当前变形算法实例

    ImVec2 start_, end_;  // 临时存储拖拽起点和终点
    bool flag_enable_selecting_points_ = false;
    bool draw_status_ = false;
    WarpingType warping_type_ = kDefault;

    // 辅助函数
    std::pair<int, int> fisheye_warping(int x, int y, int width, int height);
    std::vector<uchar> bilinear_interpolate(float x, float y, const Image& src);
};
} // namespace USTC_CG