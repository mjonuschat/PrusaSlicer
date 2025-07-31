///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/SidebarPrint.hpp"

#include "Slic3r/App/Yoga/RectangleButton.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/InputTextWithSpin.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/App/Yoga/RadioButton.hpp"
#include "Slic3r/App/Yoga/RadioExtruder.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <Slic3r/Log.hpp>

#include <imgui/imgui.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;

namespace Slic3r::App {

SidebarPrint::SidebarPrint(Biz::ProjectInteractor& project_interactor) :
    Window("sidebar_print"),
    m_project_interactor(project_interactor),
    m_print_settings_dialog(project_interactor)
{
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

    m_print_settings_dialog.attach_to_item(this, Position::Left, 20);
    m_print_settings_dialog.callbacks().closed = [this]() {
        for (AbstractButton* button : m_group_print_tools->buttons()) {
            button->set_checked(false);
        }
    };
    m_print_settings_dialog.dialog_callbacks().tab_selected = [this](size_t tab_index) {
        if (tab_index == 0) {
            m_settings_set_btn->set_checked(true);
        } else {
            m_tool_head_list_view->item_at(tab_index - 1)->cog_button()->set_checked(true);
        }
    };

    Item* layer_height_row = m_content_area->emplace_back<Item>();
    layer_height_row->set_flex_shrink(0);
    Rectangle* text_rect = layer_height_row->emplace_back<Rectangle>();
    text_rect->set_fill(ImColor(41, 41, 41));
    text_rect->set_align_items(YGAlignCenter);
    text_rect->set_flags(ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomLeft);
    text_rect->set_padding(Paddings(5, 0));
    text_rect->emplace_back<Text>("Print");

    m_combo_print = layer_height_row
                        ->emplace_back<ComboBoxListViewSelection<Biz::Preset::PresetItem>>();
    m_combo_print->set_get_name_fn([](const Biz::Preset::PresetItem* item) -> std::string {
        return item->name;
    });
    m_combo_print->set_source_list(&m_project_interactor.preset_interactor().print_presets().items());
    m_combo_print->set_flex_grow(1);
    m_combo_print->callbacks().selection_changed = [this](int print_index) {
        if (print_index >= 0) {
            m_project_interactor.preset_interactor().select_print_preset(
                m_project_interactor.preset_interactor().print_presets().items().at(print_index).id
            );
        }
    };
    m_project_interactor.preset_interactor()
        .print_presets()
        .add_listener<Biz::IListSelectionChangedListener>(m_combo_print);

    m_settings_set_btn = layer_height_row->emplace_back<LayoutButton>("", Render::Icon::Cog);
    m_settings_set_btn->set_checkable(true);
    m_group_print_tools = std::make_shared<ButtonGroup>();
    m_group_print_tools->set_always_checked(false);
    m_group_print_tools->insert_button(m_settings_set_btn);

    m_tool_head_list_view = m_content_area->emplace_back<ToolHeadListView>(
        Yoga::ViewFactory<
            SidebarToolHeadRow,
            Biz::Preset::PresetItemObservableList,
            std::weak_ptr<Yoga::ButtonGroup>,
            Biz::ProjectInteractor&>{m_group_print_tools, m_project_interactor}
    );
    m_tool_head_list_view->set_orientation(Orientation::Vertical);
    m_tool_head_list_view->set_gap(5);
    m_tool_head_list_view->set_flex_shrink(0);
    m_tool_head_list_view->set_source_list(&m_project_interactor.preset_interactor().tool_presets());

    m_group_print_tools->callbacks().checked_changed =
        [this](AbstractButton* current_checked, AbstractButton* last_checked) {
        if (current_checked) {
            m_print_settings_dialog.open();

            // This is really ugly, refactor this once we will settle on the final-ish design
            if (current_checked == m_settings_set_btn) {
                m_print_settings_dialog.set_current_tab(0);
            } else {
                for (size_t tool_index = 0; tool_index < m_tool_head_list_view->item_count();
                     ++tool_index)
                {
                    AbstractButton* cog_button = dynamic_cast<SidebarToolHeadRow*>(
                                                     m_tool_head_list_view->item_at(tool_index)
                    )
                                                     ->cog_button();
                    if (cog_button == current_checked) {
                        m_print_settings_dialog.set_current_tab(1 + tool_index);
                        break;
                    }
                }
            }
        } else {
            m_print_settings_dialog.close();
        }
    };

    const Vec2f button_size{24.f, 24.f};
    constexpr float gap_size = 10;

    add_separator();

    std::unique_ptr<Item> extruders_selector = std::make_unique<Item>();
    extruders_selector->set_gap(gap_size);

    size_t extruer_id = 0;
    for (const ImColor& color : std::initializer_list<ImColor>{
             ImColor{250, 100, 24},
             ImColor{189, 1, 60},
             ImColor{112, 193, 64},
             ImColor{225, 249, 104}
         })
    {
        RadioExtruder* radio_btn = extruders_selector->emplace_back<RadioExtruder>(++extruer_id, color);
        radio_btn->set_checkable(true);
        if (extruer_id == 1)
            radio_btn->set_checked(true);
        radio_btn->set_border_width(2);
        radio_btn->set_min_size(button_size);
        m_group_extruder.insert_button(radio_btn);
    }
    add_row(m_content_area, "Extruders", std::move(extruders_selector));

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
    m_combo_tools = m_content_area->emplace_back<ComboBox>(
        std::initializer_list<std::string>{"All tools", "Tool 1", "Tool 2", "Tool 3", "Tool 4"}
    );

    m_favorite_params_layout = m_content_area->emplace_back<Item>();
    m_favorite_params_layout->set_flex_shrink(0);
    m_favorite_params_layout->set_orientation(Orientation::Vertical);

    create_favorite_params_page(m_favorite_params_layout); // Tool 4
}

void SidebarPrint::create_favorite_params_page(Item* container)
{
    Item* page = container->emplace_back<Item>();
    page->set_orientation(Orientation::Vertical);
    page->set_gap(5);

    std::unique_ptr<InputTextField> input_text = std::make_unique<InputTextField>();
    m_input_text_perimeters                    = input_text.get();
    m_input_text_perimeters->set_validator(std::make_unique<IntValidator>());
    m_input_text_perimeters->set_flags(ImGuiInputTextFlags_CharsDecimal);
    add_row(page, "Perimeters", std::move(input_text));

    add_separator();

    add_row(page, "Infill", std::make_unique<Item>());

    std::unique_ptr<ComboBox> combo_density = std::make_unique<ComboBox>(
        std::initializer_list<std::string>{"0%", "5%", "10%", "15%", "20%", "30%"}
    );
    combo_density->set_editable(true);
    m_combo_density = combo_density.get();
    add_row(page, "Density", std::move(combo_density));

    std::unique_ptr<InputTextWithSpin> spin = std::make_unique<InputTextWithSpin>(
        std::make_unique<IntValidator>(-3, 20),
        1,
        8
    );
    InputTextWithSpin* m_spin_perimeters = spin.get();
    m_spin_perimeters->set_flags(ImGuiInputTextFlags_CharsDecimal);
    add_row(page, "Spin test", std::move(spin));

    std::unique_ptr<InputTextWithSpin> spin_double = std::make_unique<InputTextWithSpin>(
        std::make_unique<DoubleValidator>(0.4, 7.8),
        0.2,
        2.
    );
    InputTextWithSpin* m_spin_double_perimeters = spin_double.get();
    m_spin_double_perimeters->set_flags(ImGuiInputTextFlags_CharsDecimal);
    add_row(page, "Spin double test", std::move(spin_double));
}

} // namespace Slic3r::App
