#pragma once

#include "libslic3r/ObjectID.hpp"
#include "libslic3r/Geometry.hpp"

namespace Slic3r::Domain {

class BedInstance : public ObjectBase
{
public:
    const Geometry::Transformation& transformation() const { return m_transformation; }
    void set_transformation(const Geometry::Transformation& transformation) { m_transformation = transformation; }
    const Transform3d& matrix() const { return m_transformation.get_matrix(); }

    bool active() const { return m_active; }
    void set_active(bool value) { m_active = value; }

    bool contour_enabled() const { return m_contour_enabled; }
    void set_contour_enabled(bool value) { m_contour_enabled = value; }

    bool print_volume_enabled() const { return m_print_volume_enabled; }
    void set_print_volume_enabled(bool value) { m_print_volume_enabled = value; }

private:
    BedInstance();

    BedInstance(const BedInstance& rhs) = delete;
    BedInstance(BedInstance&& rhs) = delete;
    BedInstance& operator=(const BedInstance& rhs) = delete;
    BedInstance& operator=(BedInstance&& rhs) = delete;

private:
    Geometry::Transformation m_transformation;
    bool m_active{ true };
    bool m_contour_enabled{ false };
    bool m_print_volume_enabled{ false };

    friend class Bed;
};

} // namespace Slic3r::Domain
