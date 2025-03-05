#pragma once
#include "shape.h"
#include <vector>

namespace USTC_CG {
class Freehand : public Shape {
   public:
    Freehand() = default;
    virtual ~Freehand() = default;

    void draw(const Config& config) const override;
    void update(float x, float y) override;
    void add_point(float x, float y);  

   private:
    std::vector<float> x_list_;  
    std::vector<float> y_list_;  
};
}  // namespace USTC_CG