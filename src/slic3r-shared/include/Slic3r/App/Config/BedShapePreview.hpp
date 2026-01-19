#pragma once

#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::App::Yoga {

class BedShapePreview : public Yoga::Rectangle
{
public:
    explicit BedShapePreview();

    void set_shape(
        const std::vector<Domain::Vec2d>& points,
        const std::vector<Domain::Vec2d>& triangles,
        const Domain::Vec2d& orig_pos
    );

    const ImColor& shape_fill() const;
    void set_shape_fill(const ImColor& fill);

private:
    void render(Vec2f pos, Vec2f size) override;

private:

    ImColor m_shape_fill = IM_COL32_WHITE;

    std::vector<Domain::Vec2d> m_points;
    std::vector<Domain::Vec2d> m_triangles;

    Domain::Vec2d m_orig_pos{Domain::Vec2d::Zero()};
};

} // namespace Slic3r::App::Yoga
