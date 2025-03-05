#include "ellipse.h"
#include <cmath>
#include <imgui.h>
#include <stdexcept>

namespace USTC_CG
{
void Ellipse::draw(const Config& config) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 center((start_point_x_ + end_point_x_) / 2, (start_point_y_ + end_point_y_) / 2);
    ImVec2 radius(std::fabs(end_point_x_ - start_point_x_) / 2, std::fabs(end_point_y_ - start_point_y_) / 2);

    if (radius.x < 0.1f || radius.y < 0.1f) {
        return;
    }

    draw_list->AddEllipse(
        ImVec2(center.x + config.bias[0], center.y + config.bias[1]),
        ImVec2(radius.x, radius.y),
        IM_COL32(
            config.line_color[0],
            config.line_color[1],
            config.line_color[2],
            config.line_color[3]),
        0.f,  // No rounding of corners
        ImDrawFlags_None,
        config.line_thickness);
}

void Ellipse::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
}

}  // namespace USTC_CG
