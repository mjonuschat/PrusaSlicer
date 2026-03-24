///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/SidebarPrint.hpp"

#include "Slic3r/App/Yoga/RectangleButton.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/App/Yoga/RadioButton.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/PrintSettingsDialog.hpp"

#include "Slic3r/App/Config/PrintToolFavoritesItem.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Preset/PresetSelectionCheck.hpp"

#include <Slic3r/Log.hpp>

#include <imgui/imgui.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;

namespace Slic3r::App {

SidebarPrint::SidebarPrint(Biz::ProjectInteractor& project_interactor, Navigator& navigator) :
    Window("SidebarPrint"),
    m_preset_changed_listener_scope(project_interactor.preset_interactor(), *this),
    m_project_interactor(project_interactor),
    m_navigator(navigator)
{
    m_print_settings_dialog = emplace_back<PrintSettingsDialog>(project_interactor, m_navigator);

    set_orientation(Orientation::Vertical);
    set_gap(5);

    Paddings pad = padding();
    pad.right    = 0;
    set_padding(pad);

    set_flex_grow(1);

    m_content_area = emplace_back<ScrollArea>();
    m_content_area->set_orientation(Orientation::Vertical);
    m_content_area->set_flex_grow(1);
    m_content_area->set_padding(Paddings(0, 0, 14, 0));
    m_content_area->set_gap(5);

    m_print_settings_dialog->attach_to_item(this, Position::Left, 20);
    m_print_settings_dialog->callbacks().closed = [this]()
    { m_settings_set_btn->set_checked(false); };

    Item* layer_height_row = m_content_area->emplace_back<Item>();
    layer_height_row->set_flex_shrink(0);
    Rectangle* text_rect = layer_height_row->emplace_back<Rectangle>();
    text_rect->set_fill(m_theme->color_imgui(Platform::Color::WindowBgAlternate));
    text_rect->set_align_items(YGAlignCenter);
    text_rect->set_flags(ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomLeft);
    text_rect->set_padding(Paddings(5, 0));
    text_rect->emplace_back<Text>("Print");

    m_combo_print =
        layer_height_row->emplace_back<ComboBoxListViewSelection<Biz::Preset::PresetItem>>();
    m_combo_print->set_get_name_fn(
        [](const Biz::Preset::PresetItem* item) -> std::string
        {
            const std::string prefix{item->runtime_only ? Biz::_u8L("(From 3mf) ") : ""};
            return prefix + item->name;
        }
    );
    m_combo_print->set_source_list(&m_project_interactor.preset_interactor().print_presets());
    m_combo_print->set_flex_grow(1);
    m_combo_print->callbacks().selection_changed = [this](int print_index)
    {
        if (print_index >= 0) {
            auto& preset_interactor = m_project_interactor.preset_interactor();
            const std::string& preset_id =
                preset_interactor.print_presets().items().at(print_index).id;
            if (Biz::Preset::PresetSelectionCheck::can_select_print_preset(
                    preset_interactor,
                    preset_id
                ))
            {
                preset_interactor.select_print_preset(preset_id);
                m_last_selected_index = print_index;
            } else {
                m_combo_print->set_current_index(m_last_selected_index);
            }
        }
    };

    m_settings_set_btn =
        layer_height_row->emplace_back<LayoutButton>(std::string{}, Render::Icon::Cog);
    m_settings_set_btn->set_checkable(true);
    m_settings_set_btn->set_height(24.f);
    m_settings_set_btn->set_self_align(YGAlignCenter);
    m_settings_set_btn->set_background_color(Platform::Color::ButtonTransparent);
    m_settings_set_btn->callbacks().action = [this]()
    {
        m_navigator.set_opened_dialog(
            m_settings_set_btn->checked() ? m_print_settings_dialog : nullptr
        );
    };

    m_tool_head_list_view = m_content_area->emplace_back<ToolHeadListView>(
        Yoga::ViewFactory<
            SidebarToolHeadRow,
            Biz::Preset::PresetItemObservableList,
            Biz::ProjectInteractor&>{m_project_interactor}
    );
    m_tool_head_list_view->set_orientation(Orientation::Vertical);
    m_tool_head_list_view->set_gap(5);
    m_tool_head_list_view->set_flex_shrink(0);
    m_tool_head_list_view->set_source_list(
        &m_project_interactor.preset_interactor().tool_presets()
    );

    update_tools_visibility();

    add_separator();

    create_favorite_params();
}

void SidebarPrint::add_separator()
{
    Separator* separator = m_content_area->emplace_back<Separator>();
    separator->set_margin(Margins(-m_padding.left, gap(), -m_padding.right, gap()));
}

void SidebarPrint::add_row(Item* container, const std::string& label, std::unique_ptr<Item> control)
{
    Item* row = container->emplace_back<Item>();
    row->set_flex_shrink(0);
    row->set_gap(5);

    Text* text = row->emplace_back<Text>(label);
    text->set_width_percent(30);
    text->set_self_align(YGAlign::YGAlignCenter);

    control->set_width_percent(70);

    row->append(std::move(control));
}

void SidebarPrint::create_favorite_params()
{
    m_favorite_params_layout = m_content_area->emplace_back<PrintToolFavoritesItem>(
        m_project_interactor.preset_interactor().print_tool_cbi(),
        m_project_interactor.preset_interactor()
    );
}

void SidebarPrint::update_tools_visibility()
{
    const bool is_multi_extruder =
        Domain::Preset::get_feature<bool>(
            m_project_interactor.preset_interactor().selected_printer_preset().hw_config.features,
            "multi_extruder"
        )
            .value_or(false);

    m_tool_head_list_view->set_visible(is_multi_extruder);
}

PrintSettingsDialog& SidebarPrint::print_settings_dialog()
{
    return *m_print_settings_dialog;
}

void SidebarPrint::on_preset_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    Biz::Preset::PresetItemType type
)
{
    if (type == Biz::Preset::PresetItemType::PrinterPreset
        && m_project_interactor.selected_project_id() == project_id
        && m_project_interactor.selected_config_container_id() == config_container_id)
    {
        update_tools_visibility();
    }
}

void SidebarPrint::on_config_container_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id
)
{
    if (m_project_interactor.selected_project_id() == project_id
        && m_project_interactor.selected_config_container_id() == config_container_id)
    {
        update_tools_visibility();
    }
}

} // namespace Slic3r::App
