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
using Slic3r::Domain::ModelVolume;

namespace Slic3r::App::Plater {

MultiMaterialPaintingGizmo::MultiMaterialPaintingGizmo(
    Render::Device& device,
    Scene::GeometryDataFactory& data_factory,
    Biz::ProjectInteractor& project_interactor,
    PlaterScenePresenter& scene_presenter
) :
    PaintOnGizmoBase(device, data_factory, project_interactor, scene_presenter)
{
    m_dialog = std::make_unique<MultiMaterialPaintingDialog>(project_interactor);
    m_dialog->set_tool_type(m_tool_type);
    m_dialog->set_brush_type(m_cursor_type);
    m_dialog->set_brush_radius(m_cursor_radius);
    m_dialog->set_smart_fill_angle(m_smart_fill_angle);
    m_dialog->set_bucket_fill_angle(m_bucket_fill_angle);
    m_dialog->set_height_range(m_height_range_z_range);
    m_dialog->set_clipping_of_view_value(0.);
    m_dialog->set_split_triangles_value(m_triangle_splitting_enabled);
    update_painting_dialog_tools();

    m_dialog->callbacks().first_brush_color_changed = [this](size_t color_idx)
    { m_first_brush_color_idx = color_idx; };

    m_dialog->callbacks().second_brush_color_changed = [this](size_t color_idx)
    { m_second_brush_color_idx = color_idx; };

    m_project_interactor.project_settings_interactor().add_listener<Biz::IColorsChangedListener>(
        this
    );
    m_project_interactor.preset_interactor().add_listener<Biz::Preset::IPresetChangedListener>(
        this
    );

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

MultiMaterialPaintingGizmo::~MultiMaterialPaintingGizmo()
{
    m_project_interactor.project_settings_interactor().remove_listener<Biz::IColorsChangedListener>(
        this
    );
    m_project_interactor.preset_interactor().remove_listener<Biz::Preset::IPresetChangedListener>(
        this
    );
}

float MultiMaterialPaintingGizmo::get_cursor_radius_min() const
{
    return MultiMaterialPaintingGizmo::CursorRadiusMin;
}

float MultiMaterialPaintingGizmo::get_cursor_edge_limit() const
{
    return this->get_cursor_radius_min() / 5.f;
}

Scene::ToolType MultiMaterialPaintingGizmo::type() const
{
    return Scene::ToolType::MultiMaterialPaintingGizmo;
}

bool MultiMaterialPaintingGizmo::enabled() const
{
    const Biz::Scene::ObjectSelection& selection =
        m_project_interactor.scene_interactor().object_selection();
    const bool whole_instance{selection.state() == Biz::Scene::SelectionState::WholeInstance};

    const Domain::SelectionId config_container_id{
        m_project_interactor.selected_config_container_id()
    };
    const Domain::Project& project{
        m_project_interactor.workbench().project(m_project_interactor.selected_project_id())
    };
    const Domain::ConfigContainer* config_container{
        project.find_config_container(config_container_id)
    };
    if (config_container == nullptr) {
        return false;
    }

    const size_t slot_count = config_container->selected_preset().hw_config.material_slot_count();
    return whole_instance
        && slot_count > 1
        && config_container->print_technology() == Domain::PrinterTechnology::FFF;
}

void MultiMaterialPaintingGizmo::register_commands(Platform::CommandRegistry& registry)
{
    registry.register_command(
        std::make_unique<Platform::FuncCommand>(
            "multi-material-painting-gizmo-switch-colors",
            [this]()
            {
                if (m_dialog.get()) {
                    m_dialog->switch_colors();
                }
            },
            Platform::FuncCommandExtraOpts{
                .keyboard_shortcuts =
                    Platform::KeyboardShortcuts{Platform::KeyboardShortcut{0, Platform::KeyCode::X}}
            }
        )
    );

    using Platform::KeyCode;

    const std::vector<std::pair<std::size_t, std::vector<KeyCode>>> shortcuts{
        {0, {KeyCode::Num1, KeyCode::Kp1}},
        {1, {KeyCode::Num2, KeyCode::Kp2}},
        {2, {KeyCode::Num3, KeyCode::Kp3}},
        {3, {KeyCode::Num4, KeyCode::Kp4}},
        {4, {KeyCode::Num5, KeyCode::Kp5}},
        {5, {KeyCode::Num6, KeyCode::Kp6}},
        {6, {KeyCode::Num7, KeyCode::Kp7}},
        {7, {KeyCode::Num8, KeyCode::Kp8}},
    };

    for (const auto& [index, key_codes] : shortcuts) {
        Platform::KeyboardShortcuts shortcuts{};
        for (const KeyCode& key_code : key_codes) {
            shortcuts.push_back(
                Platform::KeyboardShortcut{
                    Platform::KeyModifiers(Platform::KeyModifier::Shift),
                    key_code
                }
            );
        }
        registry.register_command(
            std::make_unique<Platform::FuncCommand>(
                "multi-material-painting-gizmo-select-primary-color-" + std::to_string(index),
                [this, index]()
                {
                    m_first_brush_color_idx = index;
                    if (m_dialog.get()) {
                        m_dialog->set_first_brush_color_index(m_first_brush_color_idx);
                    }
                },
                Platform::FuncCommandExtraOpts{.keyboard_shortcuts = shortcuts}
            )
        );
    }
}

std::unique_ptr<GizmoWindow> MultiMaterialPaintingGizmo::release_ui_window()
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
    const bool result{model_volume.mm_segmentation_facets.set_data(triangle_selector.serialize())};

    m_project_interactor.undo_provider().take_snapshot(Biz::UndoSnapshotType::MMPaintingStroke);
    return result;
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

ColorRGBA MultiMaterialPaintingGizmo::create_default_painting_color(
    const ModelVolume& model_volume
) const
{
    const int extruder_idx = std::max(0, model_volume.extruder_id() - 1);
    if (extruder_idx < static_cast<int>(m_painting_colors.size())) {
        return m_painting_colors[extruder_idx];
    }

    return PaintOnGizmoBase::create_default_painting_color(model_volume);
}

std::vector<Domain::ColorRGBA> MultiMaterialPaintingGizmo::create_painting_colors() const
{
    const auto& settings_interactor{m_project_interactor.project_settings_interactor()};
    const auto rgb_colors{
        settings_interactor.get_colors(m_project_interactor.selected_config_container_id())
    };

    std::vector<ColorRGBA> result;
    result.reserve(rgb_colors.size());
    std::ranges::transform(
        rgb_colors,
        std::back_inserter(result),
        [](const Domain::ColorRGB& color)
        { return Domain::ColorRGBA{color.r(), color.g(), color.b(), 1.0f}; }
    );
    return result;
}

void MultiMaterialPaintingGizmo::update_painting_dialog_tools()
{
    if (!m_dialog.get()) {
        return;
    }

    const auto& selected{m_project_interactor.preset_interactor().selected_printer_preset()};
    const std::size_t slot_count{selected.hw_config.material_slot_count()};

    if (slot_count <= 1) {
        return;
    }

    const std::vector<Domain::ColorRGBA> colors{create_painting_colors()};

    const bool count_changed{m_painting_colors.size() != colors.size()};

    m_painting_colors = colors;

    if (count_changed) {
        m_first_brush_color_idx  = 0;
        m_second_brush_color_idx = 1;
    }

    m_dialog->set_painting_colors(m_painting_colors);
    m_dialog->set_first_brush_color_index(m_first_brush_color_idx);
    m_dialog->set_second_brush_color_index(m_second_brush_color_idx);
}

void MultiMaterialPaintingGizmo::on_colors_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId /*config_container_id*/,
    const std::vector<Domain::ColorRGB>& /*colors*/
)
{
    update_painting_dialog_tools();

    for (auto& wrapper : m_triangle_selector_wrappers) {
        wrapper.update_painted_geometry(m_device);
    }
}

void MultiMaterialPaintingGizmo::on_preset_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    Biz::Preset::PresetItemType type
)
{
    update_painting_dialog_tools();
}

} // namespace Slic3r::App::Plater
