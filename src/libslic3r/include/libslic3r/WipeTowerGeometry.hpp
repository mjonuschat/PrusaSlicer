#pragma once

#include <vector>
#include "Slic3r/Domain/ExPolygon.hpp"
#include "Slic3r/Domain/Model.hpp"

namespace Slic3r::Biz::Print {
struct ZDepth
{
    double z{};
    double depth{};
};

struct WipeTowerGeometry
{
    std::vector<ZDepth> depths;
    double fallback_depth{};
    double fallback_height{};
    double width{};
    double cone_radius{};
    double cone_x_scale{};
    double brim_width{};

    [[nodiscard]] double get_height() const;

    [[nodiscard]] Domain::ExPolygon get_outline(
        const Domain::ModelWipeTower& model_wipe_tower
    ) const;

    [[nodiscard]] Domain::BoundingBox3d get_bounding_box(
        const Domain::ModelWipeTower& model_wipe_tower
    ) const;

    [[nodiscard]] Domain::Vec2d get_center(
        const Domain::ModelWipeTower& model_wipe_tower
    ) const;
};
} // namespace Slic3r::Biz::Print
