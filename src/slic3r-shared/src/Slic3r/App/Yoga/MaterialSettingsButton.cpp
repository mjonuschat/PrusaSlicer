#include "Slic3r/App/Yoga/MaterialSettingsButton.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ColorPickerButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Algorithms/Color.hpp"

#include "libslic3r/format.hpp"

using namespace Slic3r::Biz;

namespace Slic3r::App::Yoga {

MaterialSettingsButton::MaterialSettingsButton(
    size_t index,
    const Biz::Preset::PresetItemObservableList& state,
    std::weak_ptr<ButtonGroup> button_group,
    Biz::ProjectInteractor& project_interactor
) :
    RectangleButton(format(_u8L("Material %1% TT"), index + 1)),
    Biz::DataObserver<Biz::Preset::PresetItemObservableList>(index, state),
    m_preset_changed_listener_scope(project_interactor.preset_interactor(), *this),
    m_colors_changed_listener_scope(project_interactor.project_settings_interactor(), *this),
    m_button_group(button_group),
    m_project_interactor(project_interactor)
{
    set_checkable(true);
    set_height(25);
    set_flex_shrink(0);
    set_allow_overlap(true);

    // invalidate vertical padding to use whole button height for separators
    set_content_padding({5.f, 0.f});

    Item* text_index = emplace_back<Text>(std::to_string(index + 1));
    text_index->set_self_align(YGAlignCenter);

    emplace_back<Separator>(Orientation::Vertical)
        ->set_fill(m_theme->color_imgui(Platform::Color::WindowBg));

    m_color_marker = emplace_back<ColorPickerButton>();
    m_color_marker->set_height(16);
    m_color_marker->set_width(16);
    m_color_marker->set_rounding(8);
    m_color_marker->set_background_border_width(1);
    m_color_marker->set_self_align(YGAlignCenter);
    m_color_marker->set_delayed_update(true);
    m_color_marker->callbacks().color_edited = [this](const ImColor& color)
    {
        m_project_interactor.project_settings_interactor().set_color_from_user(
            m_project_interactor.selected_config_container_id(),
            m_index,
            Biz::Algorithms::Color::encode_color({color.Value.x, color.Value.y, color.Value.z})
        );
    };

    m_material_name = emplace_back<Text>(std::string{});
    m_material_name->set_flex_grow(1.f);
    m_material_name->set_self_align(YGAlignCenter);
    m_material_name->set_wrap_mode(Text::WrapMode::WrapElide);

    emplace_back<Separator>(Orientation::Vertical)
        ->set_fill(m_theme->color_imgui(Platform::Color::WindowBg));

    m_nozzle = emplace_back<Text>(std::string{});
    m_nozzle->set_self_align(YGAlignCenter);

    on_data_update();

    m_button_group.lock()->insert_button(this);
}

MaterialSettingsButton::~MaterialSettingsButton()
{
    if (!m_button_group.expired()) {
        m_button_group.lock()->remove_button(this);
    }
}

void MaterialSettingsButton::set_color(const ImColor& color)
{
    m_color_marker->set_color(color);
}

void MaterialSettingsButton::set_nozzle(const std::string& nozzle)
{
    m_nozzle->set_text(nozzle);
}

void MaterialSettingsButton::on_data_update()
{
    on_list_selection_changed(m_state->selected_index());
}

void MaterialSettingsButton::on_list_selection_changed(Domain::SelectionId new_selection)
{
    const_cast<Biz::Preset::PresetItemObservableList*>(m_state)
        ->add_listener<Biz::IListSelectionChangedListener>(this);

    const Biz::Preset::PresetItem& preset_item = m_state->items().at(new_selection);

    const std::string prefix{preset_item.runtime_only ? _u8L("(From 3mf) ") : ""};
    set_material_name(prefix + preset_item.name);
    // Color is not set here - it comes exclusively via the on_colors_changed listener
    // from ProjectSettingsInteractor. Use a default that nobody should see.
    set_color(m_theme->color_imgui(Platform::Color::AccentPrimary));

    const auto& hw_config =
        m_project_interactor.preset_interactor().selected_printer_preset().hw_config;
    auto mat_it           = Domain::Preset::MaterialIterator::from_slot_index(hw_config, m_index);
    const auto tool_index = mat_it.tool_index();
    const Biz::Preset::ToolConfigItemObservableList& tool_config_item_ol =
        m_project_interactor.preset_interactor().tool_items().at(tool_index);
    set_nozzle(tool_config_item_ol.items().at(tool_config_item_ol.selected_index()).name);
}

void MaterialSettingsButton::on_hw_item_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    Biz::Preset::HwItemType type
)
{
    if (m_project_interactor.selected_project_id() == project_id
        && m_project_interactor.selected_config_container_id() == config_container_id
        && type == Biz::Preset::HwItemType::ToolItem)
    {
        const auto& hw_config =
            m_project_interactor.preset_interactor().selected_printer_preset().hw_config;
        auto mat_it = Domain::Preset::MaterialIterator::from_slot_index(hw_config, m_index);
        const auto tool_index = mat_it.tool_index();
        const Biz::Preset::ToolConfigItemObservableList& tool_config_item_ol =
            m_project_interactor.preset_interactor().tool_items().at(tool_index);
        set_nozzle(tool_config_item_ol.items().at(tool_config_item_ol.selected_index()).name);
    }
}

void MaterialSettingsButton::on_colors_changed(
    Domain::SelectionId config_container_id,
    const std::vector<Domain::ColorRGB>& colors
)
{
    // we are only interested in the selected config container
    if (m_project_interactor.selected_config_container_id() != config_container_id) {
        return;
    }

    ASSERT(colors.size() > m_index);

    const Domain::ColorRGB& color = colors.at(m_index);
    set_color({color.r(), color.g(), color.b()});
}

void MaterialSettingsButton::set_material_name(const std::string& name)
{
    m_material_name->set_text(name);
}

} // namespace Slic3r::App::Yoga
