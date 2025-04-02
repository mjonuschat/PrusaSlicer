#pragma once

#include "Bed.hpp"
#include "Slic3r/Assert.hpp"
#include "libslic3r/ObjectID.hpp"
#include "libslic3r/Geometry.hpp"

namespace Slic3r {
class ModelInstance;
}

namespace Slic3r::Domain {
// TODO: move this to better place
using ModelInstanceList = std::vector<ModelInstance*>;

class Bed;
class BedInstance : public ObjectBase
{
public:

    explicit BedInstance(const Bed& bed) : m_bed(bed) {}

    const Geometry::Transformation& transformation() const { return m_transformation; }
    void set_transformation(const Geometry::Transformation& transformation) { m_transformation = transformation; }
    const Transform3d& matrix() const { return m_transformation.get_matrix(); }

    bool active() const { return m_active; }
    void set_active(bool value) { m_active = value; }

    bool contour_enabled() const { return m_contour_enabled; }
    void set_contour_enabled(bool value) { m_contour_enabled = value; }

    bool print_volume_enabled() const { return m_print_volume_enabled; }
    void set_print_volume_enabled(bool value) { m_print_volume_enabled = value; }

    const Bed& bed() const { return m_bed; }

    bool contains(const BoundingBox2d& bounds) const
    {
        Vec3d pos = m_transformation.get_offset();
        return m_bed.contains(Vec2d{pos.x(), pos.y()}, bounds);
    }

    const ModelInstanceList& model_instances() const { return m_model_instances; }
    ModelInstanceList& model_instances() { return m_model_instances; }

private:
    const Bed& m_bed;
    Geometry::Transformation m_transformation;
    ModelInstanceList m_model_instances;
    bool m_active{ false };
    bool m_contour_enabled{ false };
    bool m_print_volume_enabled{ false };
};

} // namespace Slic3r::Domain
