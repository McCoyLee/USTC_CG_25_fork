#pragma once
#include <vector>
#include "shape.h"

namespace USTC_CG
{
class Polygon : public Shape
{
   public:
    Polygon() = default;

    Polygon(const std::vector<float>& x_list, const std::vector<float>& y_list);
    virtual ~Polygon() = default;

    void draw(const Config& config) const override;
    void update(float x, float y) override;
    void add_control_point(float x, float y);
    void close_polygon();

    std::vector<float>& get_x_list() { return x_list_; }
    std::vector<float>& get_y_list() { return y_list_; }
    bool is_closed() const { return is_closed_; }
    
    private:
    std::vector<float> x_list_;
    std::vector<float> y_list_;
    bool is_closed_ = false;
};
}