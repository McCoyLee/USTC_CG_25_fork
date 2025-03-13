#include "warping_widget.h"
#include "warper/IDW_warper.h"
#include "warper/RBF_warper.h"
#include "warper/nn_warper.h"

#include <cmath>
#include <iostream>

namespace USTC_CG
{
using uchar = unsigned char;

WarpingWidget::WarpingWidget(const std::string& label, const std::string& filename)
    : ImageWidget(label, filename)
{
    if (data_)
        back_up_ = std::make_shared<Image>(*data_);
}

void WarpingWidget::draw()
{
    // Draw the image
    ImageWidget::draw();
    // Draw the canvas
    if (flag_enable_selecting_points_)
        select_points();
}

void WarpingWidget::invert()
{
    for (int i = 0; i < data_->width(); ++i)
    {
        for (int j = 0; j < data_->height(); ++j)
        {
            const auto color = data_->get_pixel(i, j);
            data_->set_pixel(
                i,
                j,
                { static_cast<uchar>(255 - color[0]),
                  static_cast<uchar>(255 - color[1]),
                  static_cast<uchar>(255 - color[2]) });
        }
    }
    // After change the image, we should reload the image data to the renderer
    update();
}
void WarpingWidget::mirror(bool is_horizontal, bool is_vertical)
{
    Image image_tmp(*data_);
    int width = data_->width();
    int height = data_->height();

    if (is_horizontal)
    {
        if (is_vertical)
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i,
                        j,
                        image_tmp.get_pixel(width - 1 - i, height - 1 - j));
                }
            }
        }
        else
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i, j, image_tmp.get_pixel(width - 1 - i, j));
                }
            }
        }
    }
    else
    {
        if (is_vertical)
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i, j, image_tmp.get_pixel(i, height - 1 - j));
                }
            }
        }
    }

    // After change the image, we should reload the image data to the renderer
    update();
}
void WarpingWidget::gray_scale()
{
    for (int i = 0; i < data_->width(); ++i)
    {
        for (int j = 0; j < data_->height(); ++j)
        {
            const auto color = data_->get_pixel(i, j);
            uchar gray_value = (color[0] + color[1] + color[2]) / 3;
            data_->set_pixel(i, j, { gray_value, gray_value, gray_value });
        }
    }
    // After change the image, we should reload the image data to the renderer
    update();
}
void WarpingWidget::warping()
{
    // HW2_TODO: You should implement your own warping function that interpolate
    // the selected points.
    // Please design a class for such warping operations, utilizing the
    // encapsulation, inheritance, and polymorphism features of C++. 

    // Create a new image to store the result
    Image warped_image(*data_);
    // Initialize the color of result image
    for (int y = 0; y < data_->height(); ++y)
    {
        for (int x = 0; x < data_->width(); ++x)
        {
            warped_image.set_pixel(x, y, { 0, 0, 0 });
        }
    }

    switch (warping_type_)
    {
        case kDefault: break;
        case kFisheye:
        {
            // Example: (simplified) "fish-eye" warping
            // For each (x, y) from the input image, the "fish-eye" warping
            // transfer it to (x', y') in the new image: Note: For this
            // transformation ("fish-eye" warping), one can also calculate the
            // inverse (x', y') -> (x, y) to fill in the "gaps".
            for (int y = 0; y < data_->height(); ++y)
            {
                for (int x = 0; x < data_->width(); ++x)
                {
                    // Apply warping function to (x, y), and we can get (x', y')
                    auto [new_x, new_y] =
                        fisheye_warping(x, y, data_->width(), data_->height());
                    // Copy the color from the original image to the result
                    // image
                    if (new_x >= 0 && new_x < data_->width() && new_y >= 0 &&
                        new_y < data_->height())
                    {
                        std::vector<unsigned char> pixel =
                            data_->get_pixel(x, y);
                        warped_image.set_pixel(new_x, new_y, pixel);
                    }
                }
            }
            break;
        }
        case kIDW: {
            if (control_points_.empty()) {
                std::cout << "No control points for IDW!" << std::endl;
                break;
            }

            current_warper_ = std::make_unique<IDWWarper>();
            current_warper_->setControlPoints(control_points_);

            // 反向映射：遍历目标图像每个像素
            for (int y = 0; y < warped_image.height(); ++y) {
                for (int x = 0; x < warped_image.width(); ++x) {
                    // 计算对应的源坐标
                    auto [src_x, src_y] = current_warper_->warp(x, y);
                    
                    // 双线性插值获取颜色
                    if (src_x >= 0 && src_x < data_->width() && 
                        src_y >= 0 && src_y < data_->height()) 
                    {
                        auto color = bilinear_interpolate(src_x, src_y, *data_);
                        warped_image.set_pixel(x, y, color);
                    }
                }
            }
            break;
        }

        case kRBF: {
            if (control_points_.empty()) {
                std::cout << "RBF变形需要至少1个控制点！" << std::endl;
                break;
            }

            current_warper_ = std::make_unique<RBFWarper>();
            // 传递图像尺寸用于边界约束
            dynamic_cast<RBFWarper*>(current_warper_.get())->setImageSize(
                data_->width(), data_->height()
            );
            current_warper_->setControlPoints(control_points_);

            // 反向映射
            for (int y = 0; y < warped_image.height(); ++y) {
                for (int x = 0; x < warped_image.width(); ++x) {
                    auto [src_x, src_y] = current_warper_->warp(x, y);
                    if (src_x >= 0 && src_x < data_->width() && 
                        src_y >= 0 && src_y < data_->height()) 
                    {
                        auto color = bilinear_interpolate(src_x, src_y, *data_);
                        warped_image.set_pixel(x, y, color);
                    }
                }
            }
            break;
        }

        case kNeuralNetwork: {  // 新增神经网络变形选项
            if (control_points_.empty()) {
                std::cout << "神经网络变形需要至少1个控制点！" << std::endl;
                break;
            }

            current_warper_ = std::make_unique<NNWarper>();
            dynamic_cast<NNWarper*>(current_warper_.get())->setImageSize(
                data_->width(), data_->height()
            );
            current_warper_->setControlPoints(control_points_);

            // 反向映射
            for (int y = 0; y < warped_image.height(); ++y) {
                for (int x = 0; x < warped_image.width(); ++x) {
                    auto [src_x, src_y] = current_warper_->warp(x, y);
                    if (src_x >= 0 && src_x < data_->width() && 
                        src_y >= 0 && src_y < data_->height()) 
                    {
                        auto color = bilinear_interpolate(src_x, src_y, *data_);
                        warped_image.set_pixel(x, y, color);
                    }
                }
            }
            break;
        }
        default: break;
    }

    *data_ = std::move(warped_image);
    update();
}
void WarpingWidget::restore()
{
    *data_ = *back_up_;
    update();
}
void WarpingWidget::set_default()
{
    warping_type_ = kDefault;
}
void WarpingWidget::set_fisheye()
{
    warping_type_ = kFisheye;
}
void WarpingWidget::set_IDW()
{
    warping_type_ = kIDW;
}
void WarpingWidget::set_RBF()
{
    warping_type_ = kRBF;
}
void WarpingWidget::set_NeuralNetwork()
{
    warping_type_ = kNeuralNetwork;
}
void WarpingWidget::enable_selecting(bool flag)
{
    flag_enable_selecting_points_ = flag;
}
void WarpingWidget::select_points() {
    /// Invisible button over the canvas to capture mouse interactions.
    ImGui::SetCursorScreenPos(position_);
    ImGui::InvisibleButton(
        label_.c_str(),
        ImVec2(
            static_cast<float>(image_width_),
            static_cast<float>(image_height_)),
        ImGuiButtonFlags_MouseButtonLeft);
    
    bool is_hovered_ = ImGui::IsItemHovered();
    ImGuiIO& io = ImGui::GetIO();

    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        // 转换为图像坐标（减去画布偏移）
        start_ = ImVec2(
            io.MousePos.x - position_.x,
            io.MousePos.y - position_.y
        );
        end_ = start_;
        draw_status_ = true;
    }

    if (draw_status_) {
        end_ = ImVec2(
            io.MousePos.x - position_.x,
            io.MousePos.y - position_.y
        );
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            // 保存为ControlPoint（坐标范围检查）
            if (start_.x >= 0 && start_.x < data_->width() && 
                start_.y >= 0 && start_.y < data_->height() &&
                end_.x >= 0 && end_.x < data_->width() && 
                end_.y >= 0 && end_.y < data_->height()) 
            {
                control_points_.emplace_back(
                    start_.x, start_.y,  // 源点
                    end_.x, end_.y       // 目标点
                );
            }
            draw_status_ = false;
        }
    }

    // 可视化：绘制所有控制点连线
    auto draw_list = ImGui::GetWindowDrawList();
    for (const auto& cp : control_points_) {
        ImVec2 s(cp.src_x + position_.x, cp.src_y + position_.y);
        ImVec2 e(cp.tar_x + position_.x, cp.tar_y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
        draw_list->AddCircleFilled(e, 4.0f, IM_COL32(0, 255, 0, 255));
    }
    if (draw_status_) {
        ImVec2 s(start_.x + position_.x, start_.y + position_.y);
        ImVec2 e(end_.x + position_.x, end_.y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
    }
}

std::vector<uchar> WarpingWidget::bilinear_interpolate(float x, float y, const Image& src) {
    // 确保坐标在图像范围内
    x = std::clamp(x, 0.0f, static_cast<float>(src.width() - 1));
    y = std::clamp(y, 0.0f, static_cast<float>(src.height() - 1));

    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = std::min(x0 + 1, src.width() - 1);
    int y1 = std::min(y0 + 1, src.height() - 1);
    float dx = x - x0;
    float dy = y - y0;

    auto c00 = src.get_pixel(x0, y0);
    auto c01 = src.get_pixel(x0, y1);
    auto c10 = src.get_pixel(x1, y0);
    auto c11 = src.get_pixel(x1, y1);

    std::vector<uchar> color(3);
    for (int i = 0; i < 3; ++i) {
        float val = (1 - dx) * (1 - dy) * c00[i] +
                    (1 - dx) * dy * c01[i] +
                    dx * (1 - dy) * c10[i] +
                    dx * dy * c11[i];
        color[i] = static_cast<uchar>(std::clamp(val, 0.0f, 255.0f));
    }
    return color;
}
void WarpingWidget::init_selections() {
    control_points_.clear();  // 清空控制点（替换旧逻辑）
}

std::pair<int, int>
WarpingWidget::fisheye_warping(int x, int y, int width, int height)
{
    float center_x = width / 2.0f;
    float center_y = height / 2.0f;
    float dx = x - center_x;
    float dy = y - center_y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // Simple non-linear transformation r -> r' = f(r)
    float new_distance = std::sqrt(distance) * 10;

    if (distance == 0)
    {
        return { static_cast<int>(center_x), static_cast<int>(center_y) };
    }
    // (x', y')
    float ratio = new_distance / distance;
    int new_x = static_cast<int>(center_x + dx * ratio);
    int new_y = static_cast<int>(center_y + dy * ratio);

    return { new_x, new_y };
}
}  // namespace USTC_CG