#include "target_image_widget.h"
#include "seamless_clone.h"

#include <cmath>

namespace USTC_CG
{
using uchar = unsigned char;

TargetImageWidget::TargetImageWidget(
    const std::string& label,
    const std::string& filename)
    : ImageWidget(label, filename)
{
    if (data_)
        back_up_ = std::make_shared<Image>(*data_);
}

void TargetImageWidget::draw()
{
    // Draw the image
    ImageWidget::draw();    
    // Invisible button for interactions
    ImGui::SetCursorScreenPos(position_);
    ImGui::InvisibleButton(
        label_.c_str(),
        ImVec2(
            static_cast<float>(image_width_),
            static_cast<float>(image_height_)),
        ImGuiButtonFlags_MouseButtonLeft);
    bool is_hovered_ = ImGui::IsItemHovered();
    // When the mouse is clicked or moving, we would adapt clone function to
    // copy the selected region to the target.

    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        mouse_click_event();
    }
    mouse_move_event();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        mouse_release_event();
    }
}

void TargetImageWidget::set_mixed_gradient()
{
    clone_type_ = kMixedGradient;
    clone();
}

void TargetImageWidget::set_source(std::shared_ptr<SourceImageWidget> source)
{
    source_image_ = source;
}

void TargetImageWidget::set_realtime(bool flag)
{
    flag_realtime_updating = flag;
}

void TargetImageWidget::restore()
{
    *data_ = *back_up_;
    update();
}

void TargetImageWidget::set_paste()
{
    clone_type_ = kPaste;
}

void TargetImageWidget::set_seamless()
{
    clone_type_ = kSeamless;
}

void TargetImageWidget::clone()
{
    // The implementation of different types of cloning
    // HW3_TODO: 
    // 1. In this function, you should at least implement the "seamless"
    // cloning labeled by `clone_type_ ==kSeamless`.
    //
    // 2. It is required to improve the efficiency of your seamless cloning to
    // achieve real-time editing. (Use decomposition of sparse matrix before
    // solve the linear system). The real-time updating (update when the mouse
    // is moving) is only available when the checkerboard is selected. 
    if (data_ == nullptr || source_image_ == nullptr ||
        source_image_->get_region_mask() == nullptr)
        return;
    // The selected region in the source image, this would be a binary mask.
    // The **size** of the mask should be the same as the source image.
    // The **value** of the mask should be 0 or 255: 0 for the background and
    // 255 for the selected region.
    std::shared_ptr<Image> mask = source_image_->get_region_mask();

    switch (clone_type_)
    {
        case USTC_CG::TargetImageWidget::kDefault: break;
        case USTC_CG::TargetImageWidget::kPaste:
        {
            restore();

            for (int x = 0; x < mask->width(); ++x)
            {
                for (int y = 0; y < mask->height(); ++y)
                {
                    int tar_x =
                        static_cast<int>(mouse_position_.x) + x -
                        static_cast<int>(source_image_->get_position().x);
                    int tar_y =
                        static_cast<int>(mouse_position_.y) + y -
                        static_cast<int>(source_image_->get_position().y);
                    if (0 <= tar_x && tar_x < image_width_ && 0 <= tar_y &&
                        tar_y < image_height_ && mask->get_pixel(x, y)[0] > 0)
                    {
                        data_->set_pixel(
                            tar_x,
                            tar_y,
                            source_image_->get_data()->get_pixel(x, y));
                    }
                }
            }
            break;
        }
        case USTC_CG::TargetImageWidget::kSeamless: 
        {
            if (source_image_ && source_image_->get_data() && data_) 
            {
                // ===== 移除错误的前置清除操作 =====
                // 直接恢复目标图像至原始状态
                *data_ = *back_up_;
                
                auto seamless_mask = source_image_->get_region_mask();
                auto source_pos = source_image_->get_position();
                int offset_x = static_cast<int>(mouse_position_.x) - 
                               static_cast<int>(source_pos.x);
                int offset_y = static_cast<int>(mouse_position_.y) - 
                               static_cast<int>(source_pos.y);
                
                // 使用备份图像作为目标基准
                SeamlessClone seamless(
                    source_image_->get_data(), 
                    back_up_,  // <- 使用原始目标图像
                    seamless_mask, 
                    offset_x, 
                    offset_y
                );
                auto result = seamless.solve(flag_realtime_updating);
                
                // 应用克隆结果
                *data_ = *result;
            }
            break;
        } 
        case USTC_CG::TargetImageWidget::kMixedGradient:{
            *data_ = *back_up_;
            auto seamless_mask = source_image_->get_region_mask();
            auto source_pos = source_image_->get_position();
            int offset_x = static_cast<int>(mouse_position_.x) - 
                         static_cast<int>(source_pos.x);
            int offset_y = static_cast<int>(mouse_position_.y) - 
                         static_cast<int>(source_pos.y);

            bool use_mixed = (clone_type_ == kMixedGradient);
            SeamlessClone seamless(
                source_image_->get_data(), 
                back_up_,
                seamless_mask, 
                offset_x, 
                offset_y,
                use_mixed  // 传递混合梯度标志
            );
            
            auto result = seamless.solve(flag_realtime_updating);
            *data_ = *result;
            break;
        }      
        default: break;
    }

    update();
}

void TargetImageWidget::mouse_click_event()
{
    edit_status_ = true;
    mouse_position_ = mouse_pos_in_canvas();
    clone();
}

void TargetImageWidget::mouse_move_event()
{
    if (edit_status_)
    {
        mouse_position_ = mouse_pos_in_canvas();
        if (flag_realtime_updating)
            clone();
    }
}

void TargetImageWidget::mouse_release_event()
{
    if (edit_status_)
    {
        edit_status_ = false;
    }
}

ImVec2 TargetImageWidget::mouse_pos_in_canvas() const
{
    ImGuiIO& io = ImGui::GetIO();
    return ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
}
}  // namespace USTC_CG