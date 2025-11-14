#include "Slic3r/App/Plater/MultiMaterialPaintingGizmo.hpp"

#include "Slic3r/App/Plater/MultiMaterialPaintingDialog.hpp"
#include "Slic3r/App/Plater/PaintOnGizmoBase.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/Clipper.hpp"
#include "Slic3r/App/Scene/ClipperPresenter.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Domain/Color.hpp"

using namespace Slic3r::App::Yoga;

using Slic3r::Domain::ColorRGBA;

namespace Slic3r::App::Plater {

MultiMaterialPaintingGizmo::MultiMaterialPaintingGizmo(
    Render::Device& device,
    Scene::GeometryDataFactory& data_factory,
    Biz::ProjectInteractor& project_interactor,
    PlaterScenePresenter& scene_presenter
) :
    PaintOnGizmoBase(device, data_factory, project_interactor, scene_presenter)
{
    m_dialog = std::make_unique<MultiMaterialPaintingDialog>(this->create_painting_colors());
    m_dialog->set_first_brush_color_index(m_first_brush_color_idx);
    m_dialog->set_second_brush_color_index(m_second_brush_color_idx);
    m_dialog->set_tool_type(m_tool_type);
    m_dialog->set_brush_type(m_cursor_type);
    m_dialog->set_brush_radius(m_cursor_radius);
    m_dialog->set_smart_fill_angle(m_smart_fill_angle);
    m_dialog->set_bucket_fill_angle(m_bucket_fill_angle);
    m_dialog->set_height_range(m_height_range_z_range);
    m_dialog->set_clipping_of_view_value(0.);
    m_dialog->set_split_triangles_value(m_triangle_splitting_enabled);

    m_dialog->callbacks().first_brush_color_changed = [this](size_t color_idx)
    { m_first_brush_color_idx = color_idx; };

    m_dialog->callbacks().second_brush_color_changed = [this](size_t color_idx)
    { m_second_brush_color_idx = color_idx; };

    m_dialog->callbacks().tool_type_changed = [this](const PaintOnGizmoBase::ToolType tool_type)
    { m_tool_type = tool_type; };

    m_dialog->callbacks().brush_shape_changed =
        [this](const Biz::Algorithms::TriangleSelector::CursorType cursor_type)
    { m_cursor_type = cursor_type; };

    m_dialog->callbacks().brush_radius_changed = [this](const double value)
    { m_cursor_radius = static_cast<float>(value); };

    m_dialog->callbacks().smart_fill_angle_changed = [this](const double value)
    { m_smart_fill_angle = static_cast<float>(value); };

    m_dialog->callbacks().bucket_fill_angle_changed = [this](const double value)
    { m_bucket_fill_angle = static_cast<float>(value); };

    m_dialog->callbacks().height_range_changed = [this](const double value)
    { m_height_range_z_range = static_cast<float>(value); };

    m_dialog->callbacks().split_triangles_value_changed = [this](const bool value)
    { m_triangle_splitting_enabled = value; };

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

    m_dialog->callbacks().painting_reset = [this]() { this->clear_all_paintings(); };
}

MultiMaterialPaintingGizmo::~MultiMaterialPaintingGizmo() = default;

Scene::ToolType MultiMaterialPaintingGizmo::type() const
{
    return Scene::ToolType::MultiMaterialPaintingGizmo;
}

std::unique_ptr<Yoga::GizmoWindow> MultiMaterialPaintingGizmo::release_ui_window()
{
    return m_dialog.release();
}

const Domain::FacetsAnnotation& MultiMaterialPaintingGizmo::get_facets_annotation(
    const Domain::ModelVolume& model_volume
) const
{
    return model_volume.mm_segmentation_facets;
}

bool MultiMaterialPaintingGizmo::set_facets_annotation(
    Domain::ModelVolume& model_volume,
    const Biz::Algorithms::TriangleSelector& triangle_selector
) const
{
    return model_volume.mm_segmentation_facets.set_data(triangle_selector.serialize());
}

Domain::TriangleSelector::TriangleStateType
MultiMaterialPaintingGizmo::get_left_button_state_type() const
{
    return Domain::TriangleSelector::TriangleStateType(m_first_brush_color_idx + 1);
}

Domain::TriangleSelector::TriangleStateType
MultiMaterialPaintingGizmo::get_right_button_state_type() const
{
    return Domain::TriangleSelector::TriangleStateType(m_second_brush_color_idx + 1);
}

ColorRGBA MultiMaterialPaintingGizmo::get_cursor_sphere_left_button_color() const
{
    ColorRGBA color = m_painting_colors[m_first_brush_color_idx];
    color.a(0.25f);
    return color;
}

ColorRGBA MultiMaterialPaintingGizmo::get_cursor_sphere_right_button_color() const
{
    ColorRGBA color = m_painting_colors[m_second_brush_color_idx];
    color.a(0.25f);
    return color;
}

void MultiMaterialPaintingGizmo::on_cursor_radius_changed(float value)
{
    m_dialog->set_brush_radius(static_cast<double>(value));
}

void MultiMaterialPaintingGizmo::on_smart_fill_angle_changed(float value)
{
    m_dialog->set_smart_fill_angle(static_cast<double>(value));
}

void MultiMaterialPaintingGizmo::on_clipping_of_view_changed(double value)
{
    m_dialog->set_clipping_of_view_value(static_cast<double>(value));
}

std::vector<Domain::ColorRGBA> MultiMaterialPaintingGizmo::create_painting_colors() const
{
    return {
        ColorRGBA::RED(),
        ColorRGBA::GREEN(),
        ColorRGBA::BLUE(),
        ColorRGBA::YELLOW(),
        ColorRGBA::MAGENTA(),
        ColorRGBA::CYAN(),
        ColorRGBA::GRAY(),
        ColorRGBA::BLACK()
    };
}

} // namespace Slic3r::App::Plater
