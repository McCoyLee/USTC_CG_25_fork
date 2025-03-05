#include "canvas_widget.h"

#include <cmath>
#include <iostream>

#include "imgui.h"
#include "shapes/line.h"
#include "shapes/rect.h"
#include "shapes/ellipse.h"
#include "shapes/polygon.h"
#include "shapes/freehand.h"

namespace USTC_CG
{
void Canvas::draw()
{
    draw_background();
    // HW1_TODO: more interaction events
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        mouse_click_event();
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        mouse_right_click_event();
    }
    mouse_move_event();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        mouse_release_event();

    draw_shapes();
}

void Canvas::set_attributes(const ImVec2& min, const ImVec2& size)
{
    canvas_min_ = min;
    canvas_size_ = size;
    canvas_minimal_size_ = size;
    canvas_max_ =
        ImVec2(canvas_min_.x + canvas_size_.x, canvas_min_.y + canvas_size_.y);
}

void Canvas::show_background(bool flag)
{
    show_background_ = flag;
}

void Canvas::set_default()
{
    draw_status_ = false;
    shape_type_ = kDefault;
}

void Canvas::set_line()
{
    draw_status_ = false;
    shape_type_ = kLine;
}

void Canvas::set_rect()
{
    draw_status_ = false;
    shape_type_ = kRect;
}

void Canvas::set_ellipse()
{
    draw_status_ = false;
    shape_type_ = kEllipse;
}

void Canvas::set_polygon()
{
    draw_status_ = false;
    shape_type_ = kPolygon;
    is_polygon_drawing_ = false;
}

void Canvas::set_freehand(){
    draw_status_ = false;
    shape_type_ = kFreehand;
    is_freehand_drawing_ = false;
}
// HW1_TODO: more shape types, implements

void Canvas::clear_shape_list()
{
    shape_list_.clear();
}

void Canvas::draw_background()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (show_background_)
    {
        // Draw background recrangle
        draw_list->AddRectFilled(canvas_min_, canvas_max_, background_color_);
        // Draw background border
        draw_list->AddRect(canvas_min_, canvas_max_, border_color_);
    }
    /// Invisible button over the canvas to capture mouse interactions.
    ImGui::SetCursorScreenPos(canvas_min_);
    ImGui::InvisibleButton(
        label_.c_str(), canvas_size_, ImGuiButtonFlags_MouseButtonLeft);
    // Record the current status of the invisible button
    is_hovered_ = ImGui::IsItemHovered();
    is_active_ = ImGui::IsItemActive();
}

void Canvas::draw_shapes()
{
    Shape::Config s = { .bias = { canvas_min_.x, canvas_min_.y } };
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->PushClipRect(canvas_min_, canvas_max_, true);
    for (const auto& shape : shape_list_)
    {
        shape->draw(s);
    }
    if (draw_status_ && current_shape_)
    {
        current_shape_->draw(s);
    }
    draw_list->PopClipRect();
}

void Canvas::mouse_click_event() {
    if (shape_type_ == kFreehand) {
        if (!is_freehand_drawing_) {
            is_freehand_drawing_ = true;
            draw_status_ = true;
            start_point_ = end_point_ = mouse_pos_in_canvas();
            current_shape_ = std::make_shared<Freehand>();
            auto freehand = std::dynamic_pointer_cast<Freehand>(current_shape_);
            if (freehand) {
                freehand->add_point(start_point_.x, start_point_.y);  // 添加第一个点
                std::cout << "Started Freehand drawing." << std::endl;
            }
        }
    }   
    else if (shape_type_ == kPolygon) {
        if (!is_polygon_drawing_) {
            is_polygon_drawing_ = true;
            draw_status_ = true;
            start_point_ = end_point_ = mouse_pos_in_canvas();  
            current_shape_ = std::make_shared<Polygon>();
            auto polygon = std::dynamic_pointer_cast<Polygon>(current_shape_);
            if (polygon) {
                polygon->add_control_point(start_point_.x, start_point_.y); 
                std::cout << "Added initial point: (" << start_point_.x << ", " << start_point_.y << ")" << std::endl;
            }
        } else {
            end_point_ = mouse_pos_in_canvas();
            auto polygon = std::dynamic_pointer_cast<Polygon>(current_shape_);
            if (polygon) {
                polygon->add_control_point(end_point_.x, end_point_.y);
                std::cout << "Added new point: (" << end_point_.x << ", " << end_point_.y << ")" << std::endl;
            }
        }
    }
    else {
        if (!draw_status_) {
            draw_status_ = true;
            start_point_ = end_point_ = mouse_pos_in_canvas();
            switch (shape_type_) {
                case kDefault:
                    break;
                case kLine:
                    current_shape_ = std::make_shared<Line>(
                        start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                    break;
                case kRect:
                    current_shape_ = std::make_shared<Rect>(
                        start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                    break;
                case kEllipse:
                    current_shape_ = std::make_shared<Ellipse>(
                        start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                    break;
                default:
                    break;
            }
        } else {
            draw_status_ = false;
            if (current_shape_) {
                shape_list_.push_back(current_shape_);
                current_shape_.reset();
            }
        }
    }
}

void Canvas::mouse_move_event() {
    if (draw_status_) {
        end_point_ = mouse_pos_in_canvas();
        if (current_shape_) {
            if (shape_type_ == kFreehand) {
                auto freehand = std::dynamic_pointer_cast<Freehand>(current_shape_);
                if (freehand) {
                    freehand->add_point(end_point_.x, end_point_.y);  // 添加新点
                }
            } else {
                current_shape_->update(end_point_.x, end_point_.y);
            }
        }
    }
}

void Canvas::mouse_release_event() {
    if (shape_type_ == kFreehand && is_freehand_drawing_) {
        if (current_shape_) {
            shape_list_.push_back(current_shape_);
            current_shape_.reset();
        }
        is_freehand_drawing_ = false;
        draw_status_ = false;
        std::cout << "Finished Freehand drawing." << std::endl;
    }
}

ImVec2 Canvas::mouse_pos_in_canvas() const
{
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouse_pos_in_canvas(
        io.MousePos.x - canvas_min_.x, io.MousePos.y - canvas_min_.y);
    return mouse_pos_in_canvas;
}

void Canvas::mouse_right_click_event() {
    if (shape_type_ == kFreehand && is_freehand_drawing_) {
        // 结束 Freehand 绘制
        if (current_shape_) {
            shape_list_.push_back(current_shape_);
            current_shape_.reset();
        }
        is_freehand_drawing_ = false;
        draw_status_ = false;
        std::cout << "Freehand drawing canceled." << std::endl;
    } else if (shape_type_ == kPolygon && is_polygon_drawing_ && current_shape_) {
        // 多边形闭合逻辑
        auto polygon = std::dynamic_pointer_cast<Polygon>(current_shape_);
        if (polygon) {
            // 删除最后一个动态更新的顶点（鼠标移动时的临时点）
            auto& x_list = polygon->get_x_list();
            auto& y_list = polygon->get_y_list();
            if (!x_list.empty()) {
                x_list.pop_back();
                y_list.pop_back();
            }
            // 标记多边形为闭合状态
            polygon->close_polygon();
            // 确保至少有3个顶点才保存
            if (x_list.size() >= 3) {
                shape_list_.push_back(current_shape_);
            }
            current_shape_.reset();
            is_polygon_drawing_ = false;
            draw_status_ = false;
            std::cout << "Polygon closed." << std::endl;
        }
    }
}

}  // namespace USTC_CG