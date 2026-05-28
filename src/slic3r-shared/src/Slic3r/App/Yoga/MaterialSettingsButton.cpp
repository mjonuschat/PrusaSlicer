#include "Slic3r/App/Yoga/MaterialSettingsButton.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ColorPickerButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Algorithms/Color.hpp"

#include "Slic3r/LegacyFormat.hpp"

using namespace Slic3r::Biz;

namespace Slic3r::App::Yoga {

MaterialSettingsButton::MaterialSettingsButton(
    size_t index,
    const Biz::Preset::PresetItemObservableList& state,
    std::weak_ptr<ButtonGroup> button_group,
    FnIndexClicked on_cog_clicked,
    Biz::ProjectInteractor& project_interactor
) :
    RectangleButton(),
    Biz::DataObserver<Biz::Preset::PresetItemObservableList>(index, state),
    m_colors_changed_listener_scope(project_interactor.project_settings_interactor(), *this),
    m_button_group(button_group),
    m_on_cog_clicked(on_cog_clicked),
    m_project_interactor(project_interactor)
{
    set_checkable(true);
    set_height(1.5_rem);
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

    Item* button_wrap = emplace_back<Item>();
    button_wrap->set_min_width(20.f);
    button_wrap->set_min_height(20.f);
    button_wrap->set_flex_shrink(0);
    m_cog_btn = button_wrap->emplace_back<LayoutButton>(
        std::string{},
        Render::Icon::Cog,
        Biz::_u8L("Show material settings")
    );
    m_cog_btn->set_self_align(YGAlignCenter);
    m_cog_btn->set_margin(-2.f);
    m_cog_btn->set_width(24.f);
    m_cog_btn->set_height(24.f);
    m_cog_btn->set_background_color(m_theme->color_imgui(Platform::Color::Transparent));
    m_cog_btn->callbacks().action = [this]()
    {
        if (!this->checked()) {
            this->set_checked(true);
        }
        m_on_cog_clicked(m_index);
    };
    update_cog_visibility();

    m_cog_btn->callbacks().hovered_changed = [this](bool) { update_cog_visibility(); };

    emplace_back<Separator>(Orientation::Vertical)
        ->set_fill(m_theme->color_imgui(Platform::Color::WindowBg));

    m_nozzle = emplace_back<Text>(std::string{});
    m_nozzle->set_self_align(YGAlignCenter);

    on_data_update();

    m_button_group.lock()->insert_button(this);

    project_interactor.preset_interactor().add_listener<Preset::IPresetChangedListener>(this);
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

    const std::vector<Domain::ColorRGB> colors =
        m_project_interactor.project_settings_interactor().get_colors(
            m_project_interactor.selected_config_container_id()
        );
    if (m_index < colors.size()) {
        const Domain::ColorRGB& color = colors[m_index];
        set_color({color.r(), color.g(), color.b()});
    } else {
        set_color(m_theme->color_imgui(Platform::Color::AccentPrimary));
    }

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

        if (m_index >= hw_config.material_slot_count()) {
            return;
        }

        auto mat_it = Domain::Preset::MaterialIterator::from_slot_index(hw_config, m_index);
        const auto tool_index = mat_it.tool_index();
        const Biz::Preset::ToolConfigItemObservableList& tool_config_item_ol =
            m_project_interactor.preset_interactor().tool_items().at(tool_index);
        set_nozzle(tool_config_item_ol.items().at(tool_config_item_ol.selected_index()).name);
    }
}

void MaterialSettingsButton::on_colors_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    const std::vector<Domain::ColorRGB>& colors
)
{
    // we are only interested in the selected config container
    if (m_project_interactor.selected_config_container_id() != config_container_id) {
        return;
    }

    // May receive a notification with fewer slots than our index during
    // project switches (buttons from the old extruder count are still alive).
    // Handle this as "no color for this slot -> leave what we have".
    if (m_index >= colors.size())
        return;

    const Domain::ColorRGB& color = colors[m_index];
    set_color({color.r(), color.g(), color.b()});
}

void MaterialSettingsButton::on_view_will_be_removed()
{
    m_project_interactor.preset_interactor().remove_listener<Preset::IPresetChangedListener>(this);
}

void MaterialSettingsButton::set_material_name(const std::string& name)
{
    m_material_name->set_text(name);
    set_tooltip(name);
}

void MaterialSettingsButton::checked_updated_internal()
{
    RectangleButton::checked_updated_internal();
    update_cog_visibility();
}

void MaterialSettingsButton::hovered_updated_internal()
{
    RectangleButton::hovered_updated_internal();
    update_cog_visibility();
}

void MaterialSettingsButton::update_cog_visibility()
{
    m_cog_btn->set_visible(this->hovered() || m_cog_btn->hovered());
}

} // namespace Slic3r::App::Yoga
