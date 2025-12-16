#pragma once

#include "Slic3r/Biz/Utils/CutUtils.hpp"

namespace Slic3r::Domaon {
class TriangleMesh;
} // namespace Slic3r::Domaon

namespace Slic3r::App::Plater {

class CutPartSelection
{
public:
    CutPartSelection() = default;
    CutPartSelection(
        const Domain::ModelObject* mo,
        const Domain::Transform3d& cut_matrix,
        int instance_idx,
        const Domain::Vec3d& center,
        const Domain::Vec3d& normal /*, const CommonGizmosDataObjects::ObjectClipper& oc*/
    );
    CutPartSelection(const Domain::ModelObject* mo, int instance_idx_in);

    ~CutPartSelection()
    {
        m_model.clear_objects();
    }

    struct Part
    {
        // GLModel glmodel;
        // MeshRaycaster raycaster;
        std::shared_ptr<const Domain::TriangleMesh> mesh;
        // Part transformation including tarnsformation of volume and instance
        Domain::Transform3d trafo;
        bool selected;
        bool is_modifier;
    };

    // void render(const Vec3d* normal, GLModel& sphere_model);
    // void toggle_selection(const Domain::Vec2d& mouse_pos);
    // void turn_over_selection();

    Domain::ModelObject* model_object()
    {
        return m_model.objects.front();
    }

    bool valid() const
    {
        return m_valid;
    }

    bool is_one_object() const;

    const std::vector<Part>& parts() const
    {
        return m_parts;
    }

    const std::vector<size_t>* get_ignored_contours_ptr() const
    {
        return (valid() ? &m_ignored_contours : nullptr);
    }

    std::vector<Biz::Cut::Part> get_cut_parts();

private:
    void add_object(const Domain::ModelObject* object);

private:
    Domain::Model m_model;
    int m_instance_idx;
    std::vector<Part> m_parts;
    bool m_valid = false;
    std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>>
        m_contour_to_parts; // for each contour, there is a vector of parts above and a vector of parts below
    std::vector<size_t>
        m_ignored_contours; // contour that should not be rendered (the parts on both sides will both be parts of the same object)

    std::vector<Domain::Vec3d> m_contour_points; // Debugging
    std::vector<std::vector<Domain::Vec3d>> m_debug_pts; // Debugging
};
} // namespace Slic3r::App::Plater
