#include "libslic3r/MultipleBeds.hpp"

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"

#include "libslic3r/BuildVolume.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/Print.hpp"

#include <cassert>
#include <algorithm>

using namespace Slic3r::Biz;

namespace Slic3r {

MultipleBeds s_multiple_beds;

namespace BedsGrid {
Index grid_coords_abs2index(GridCoords coords) {
    coords = {std::abs(coords[0]), std::abs(coords[1])};

    const int x{coords[0] + 1};
    const int y{coords[1] + 1};
    const int a{std::max(x, y)};

    if (x == a && y == a) {
        return a*a - 1;
    } else if (x == a) {
        return a*a - 2 * (a - 1) + coords[1] - 1;
    } else {
        assert(y == a);
        return a*a - (a - 1) + coords[0] - 1;
    }
}

const int quadrant_offset{std::numeric_limits<int>::max() / 4};

Index grid_coords2index(const GridCoords &coords) {
    const int index{grid_coords_abs2index(coords)};

    if (index >= quadrant_offset) {
        throw std::runtime_error("Object is too far from center!");
    }

    if (coords[0] >= 0 && coords[1] >= 0) {
        return index;
    } else if (coords[0] >= 0 && coords[1] < 0) {
        return quadrant_offset + index;
    } else if (coords[0] < 0 && coords[1] >= 0) {
        return 2*quadrant_offset + index;
    } else {
        return 3*quadrant_offset + index;
    }
}

GridCoords index2grid_coords(Index index) {
    if (index < 0) {
        throw std::runtime_error{"Negative bed index cannot be translated to coords!"};
    }

    const int quadrant{index / quadrant_offset};
    index = index % quadrant_offset;

    GridCoords result{0, 0};
    if (index == 0) {
        return result;
    }

    int id = index;
    ++id;
    int a = 1;
    while ((a+1)*(a+1) < id)
        ++a;
    id = id - a*a;
    result[0]=a;
    result[1]=a;
    if (id <= a)
        result[1] = id-1;
    else
        result[0] = id-a-1;

    if (quadrant == 1) {
        result[1] = -result[1];
    } else if (quadrant == 2) {
        result[0] = -result[0];
    } else if (quadrant == 3) {
        result[1] = -result[1];
        result[0] = -result[0];
    } else if (quadrant != 0){
        throw std::runtime_error{"Impossible bed index > max int!"};
    }
    return result;
}
}

Vec3d MultipleBeds::get_bed_translation(int id) const
{
    if (id == 0)
        return Vec3d::Zero();
    int x = 0;
    int y = 0;
    if (m_legacy_layout)
        x = id;
    else {
        BedsGrid::GridCoords coords{BedsGrid::index2grid_coords(id)};
        x = coords[0];
        y = coords[1];
    }

    // As for the m_legacy_layout switch, see comments at definition of bed_gap_relative.
    Vec2d  gap = bed_gap();
    double gap_x = (m_legacy_layout ? Algorithms::BoundingBox::sizes(m_build_volume_bb).x() * (2./10.) : gap.x());
    return Vec3d(x * (Algorithms::BoundingBox::sizes(m_build_volume_bb).x() + gap_x),
                 y * (Algorithms::BoundingBox::sizes(m_build_volume_bb).y() + gap.y()), // When using legacy layout, y is zero anyway.
                 0.);
}


Vec2d MultipleBeds::bed_gap() const
{
    // This is the only function that defines how far apart should the beds be. Used in scene and arrange.
    // Note that the spacing is momentarily switched to legacy value of 2/10 when a project is loaded.
    // Slicers before 2.9.0 used this value for arrange, and there are existing projects with objects spaced that way (controlled by the m_legacy_layout flag).
    
    // TOUCHING THIS WILL BREAK LOADING OF EXISTING PROJECTS !!!

    double gap = std::min(100., Algorithms::BoundingBox::sizes(m_build_volume_bb).norm() * (3./10.));
    return Vec2d::Ones() * gap;
}


Vec2crd MultipleBeds::get_bed_gap() const {
    return scaled(Vec2d{bed_gap() / 2.0});
};


}

