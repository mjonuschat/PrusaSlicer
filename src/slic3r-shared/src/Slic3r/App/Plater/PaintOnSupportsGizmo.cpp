#include "Slic3r/App/Plater/PaintOnSupportsGizmo.hpp"

#include "Slic3r/App/Plater/PaintOnGizmoBase.hpp"
#include "Slic3r/App/Plater/PaintOnSupportsDialog.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/Clipper.hpp"
#include "Slic3r/App/Scene/ClipperPresenter.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Domain/ModelVolume.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;

using Slic3r::Biz::Algorithms::TriangleSelector;
using Slic3r::Domain::ModelInstance;
using Slic3r::Domain::ModelObject;
using Slic3r::Domain::ModelVolume;
using Slic3r::Domain::Transform3d;
using Slic3r::Domain::Vec3d;
using Slic3r::Domain::Vec3f;

namespace Slic3r::App::Plater {

PaintOnSupportsGizmo::PaintOnSupportsGizmo(
    Render::Device& device,
    Scene::GeometryDataFactory& data_factory,
    Biz::ProjectInteractor& project_interactor,
    PlaterScenePresenter& scene_presenter
) :
    PaintOnGizmoBase(device, data_factory, project_interactor, scene_presenter)
{
    m_dialog = std::make_unique<PaintOnSupportsDialog>();
    m_dialog->set_tool_type(m_tool_type);
    m_dialog->set_brush_type(m_cursor_type);
    m_dialog->set_brush_radius(m_cursor_radius);
    m_dialog->set_smart_fill_angle(m_smart_fill_angle);
    m_dialog->set_clipping_of_view_value(0.);
    m_dialog->set_highlight_overhangs_angle(m_highlight_by_angle_threshold_deg);
    m_dialog->set_paint_on_overhangs_only_value(m_paint_on_overhangs_only);
    m_dialog->set_split_triangles_value(m_triangle_splitting_enabled);

    m_dialog->callbacks().tool_type_changed = [this](const PaintOnGizmoBase::ToolType tool_type)
    { m_tool_type = tool_type; };

    m_dialog->callbacks().brush_shape_changed =
        [this](const Biz::Algorithms::TriangleSelector::CursorType cursor_type)
    { m_cursor_type = cursor_type; };

    m_dialog->callbacks().brush_radius_changed = [this](const double value)
    { m_cursor_radius = static_cast<float>(value); };

    m_dialog->callbacks().smart_fill_angle_changed = [this](const double value)
    { m_smart_fill_angle = static_cast<float>(value); };

    m_dialog->callbacks().clipping_of_view_value_changed = [this](double value)
    {
        m_clipping_plane_presenter.set_position_by_ratio(value, true);
        this->update_clipping_plane();
    };

    m_dialog->callbacks().clipping_of_view_reset_direction = [this]()
    {
        m_clipping_plane_presenter.set_position_by_ratio(-1, false);
        this->update_clipping_plane();
    };

    m_dialog->callbacks().highlight_overhangs_angle_changed = [this](double value)
    {
        m_highlight_by_angle_threshold_deg = static_cast<float>(value);
        this->update_overhang_detection();
    };

    m_dialog->callbacks().overhangs_enforced = [this]()
    {
        this->select_facets_by_angle(m_highlight_by_angle_threshold_deg);

        m_highlight_by_angle_threshold_deg = 0.f;
        m_dialog->set_highlight_overhangs_angle(m_highlight_by_angle_threshold_deg);
    };

    m_dialog->callbacks().paint_on_overhangs_only_value_changed = [this](const bool value)
    { m_paint_on_overhangs_only = value; };

    m_dialog->callbacks().split_triangles_value_changed = [this](const bool value)
    { m_triangle_splitting_enabled = value; };

    m_dialog->callbacks().automatic_painting = [this]() { this->auto_generate_support_painting(); };

    m_dialog->callbacks().painting_reset = [this]() { this->clear_all_paintings(); };
}

PaintOnSupportsGizmo::~PaintOnSupportsGizmo() = default;

Scene::ToolType PaintOnSupportsGizmo::type() const
{
    return Scene::ToolType::PaintOnSupportsGizmo;
}

GizmoWindowPtr PaintOnSupportsGizmo::release_ui_window()
{
    return m_dialog.release();
}

const Domain::FacetsAnnotation& PaintOnSupportsGizmo::get_facets_annotation(
    const Domain::ModelVolume& model_volume
) const
{
    return model_volume.supported_facets;
}

bool PaintOnSupportsGizmo::set_facets_annotation(
    Domain::ModelVolume& model_volume,
    const Biz::Algorithms::TriangleSelector& triangle_selector
) const
{
    const bool result{model_volume.supported_facets.set_data(triangle_selector.serialize())};
    m_project_interactor.undo_provider().take_snapshot(
        Biz::UndoSnapshotType::PaintOnSupportsStroke
    );
    return result;
}

Domain::TriangleSelector::TriangleStateType PaintOnSupportsGizmo::get_left_button_state_type() const
{
    return Domain::TriangleSelector::TriangleStateType::ENFORCER;
}

Domain::TriangleSelector::TriangleStateType
PaintOnSupportsGizmo::get_right_button_state_type() const
{
    return Domain::TriangleSelector::TriangleStateType::BLOCKER;
}

void PaintOnSupportsGizmo::on_cursor_radius_changed(float value)
{
    m_dialog->set_brush_radius(static_cast<double>(value));
}

void PaintOnSupportsGizmo::on_smart_fill_angle_changed(float value)
{
    m_dialog->set_smart_fill_angle(static_cast<double>(value));
}

void PaintOnSupportsGizmo::on_clipping_of_view_changed(double value)
{
    m_dialog->set_clipping_of_view_value(static_cast<double>(value));
}

void PaintOnSupportsGizmo::select_facets_by_angle(const float threshold_deg)
{
    const float threshold = (std::numbers::pi_v<float> / 180.f) * threshold_deg;

    for (const PaintableVolume& paintable_volume : m_paintable_volumes) {
        const size_t volume_idx             = &paintable_volume - &m_paintable_volumes.front();
        const ModelObject& model_object     = paintable_volume.model_object;
        const ModelInstance& model_instance = paintable_volume.model_instance;
        const ModelVolume& model_volume     = paintable_volume.model_volume;
        TriangleSelectorRenderWrapper& triangle_selector_wrappers =
            m_triangle_selector_wrappers[volume_idx];
        TriangleSelector& triangle_selector = triangle_selector_wrappers.triangle_selector();

        const Transform3d trafo_matrix =
            model_instance.get_matrix_no_offset() * model_volume.get_matrix_no_offset();
        const Vec3f down = (trafo_matrix.inverse() * (-Vec3d::UnitZ())).cast<float>().normalized();
        const Vec3f limit =
            (trafo_matrix.inverse() * Vec3d(std::sin(threshold), 0, -std::cos(threshold)))
                .cast<float>()
                .normalized();
        const float dot_limit = limit.dot(down);

        // Now calculate dot product of vert_direction and facets' normals.
        const indexed_triangle_set& its = model_volume.mesh().its;
        for (const stl_triangle_vertex_indices& face : its.indices) {
            if (Algorithms::TriangleMesh::its_face_normal(its, face).dot(down) > dot_limit) {
                const size_t facet_idx = &face - &its.indices.front();
                triangle_selector.set_facet(
                    facet_idx,
                    Domain::TriangleSelector::TriangleStateType::ENFORCER
                );
            }
        }

        triangle_selector_wrappers.update_painted_geometry(m_device);
    }

    this->apply_painting_to_model();
}

void PaintOnSupportsGizmo::auto_generate_support_painting()
{
    // TODO: Automatic support painting isn't implemented yet, resolve it later.
}

} // namespace Slic3r::App::Plater
