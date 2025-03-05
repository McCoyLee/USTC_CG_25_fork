#include "polygon.h"
#include <imgui.h>
#include <iostream>

using namespace std;
namespace USTC_CG {
Polygon::Polygon(const std::vector<float>& x_list, const std::vector<float>& y_list)
    : x_list_(x_list), y_list_(y_list) {}


void Polygon::draw(const Config& config) const {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (x_list_.size() < 2) return;

    for (size_t i = 0; i < x_list_.size() - 1; ++i) {
        draw_list->AddLine(
            ImVec2(config.bias[0] + x_list_[i], config.bias[1] + y_list_[i]),
            ImVec2(config.bias[0] + x_list_[i + 1], config.bias[1] + y_list_[i + 1]),
            IM_COL32(
                config.line_color[0],
                config.line_color[1],
                config.line_color[2],
                config.line_color[3]),
            config.line_thickness);
    }

    if (is_closed_ && x_list_.size() >= 3) {
        draw_list->AddLine(
            ImVec2(config.bias[0] + x_list_.back(), config.bias[1] + y_list_.back()),
            ImVec2(config.bias[0] + x_list_.front(), config.bias[1] + y_list_.front()),
            IM_COL32(
                config.line_color[0],
                config.line_color[1],
                config.line_color[2],
                config.line_color[3]),
            config.line_thickness);
    }
}

void Polygon::update(float x, float y) {
    if (x_list_.empty()) {
        x_list_.push_back(x);
        y_list_.push_back(y);
    } else {
        x_list_.back() = x;
        y_list_.back() = y;
    }
}

void Polygon::add_control_point(float x, float y) {
    x_list_.push_back(x);
    y_list_.push_back(y);
}

void Polygon::close_polygon(){
    is_closed_ = true;
}
}  // namespace USTC_CG