#include "Slic3r/App/SidebarPrint.hpp"

#include "Slic3r/App/Yoga/RectangleButton.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

#include <Slic3r/Log.hpp>

#include <imgui/imgui.h>

namespace Slic3r::App {

using namespace Yoga;

SidebarPrint::SidebarPrint() : Window("sidebar_print")
{
    set_orientation(Orientation::Vertical);
    set_gap(5);
    set_flex_grow(1);

    m_settings_set_btn =
        emplace_back<LayoutButton>("Balanced settings", Render::Icon::SettingsSet, "Tooltip text");
    m_settings_set_btn->align_content(RectangleButton::Align::Left);
    m_settings_set_btn->set_checkable(true);
    m_settings_set_btn->set_background_color(ImColor(41, 41, 41));
    m_settings_set_btn->callbacks().action = []() {
        // ToDo open first settings dialog
    };

    add_separator();

    std::unique_ptr<InputTextField> input_text = std::make_unique<InputTextField>();
    m_input_text_perimeters = input_text.get();
    m_input_text_perimeters->set_validator(std::make_unique<IntValidator>());
    m_input_text_perimeters->set_flags(ImGuiInputTextFlags_CharsDecimal);
    add_row("Perimeters", std::move(input_text));
}

void SidebarPrint::add_separator()
{
    Separator* separator = emplace_back<Separator>();
    separator->set_margin(Margins(-m_padding.left, 0, -m_padding.right, 0));
}

void SidebarPrint::add_row(const std::string& label, std::unique_ptr<Item> control)
{
    Item* row = emplace_back<Item>();
    row->set_gap(5);

    Text* text = row->emplace_back<Text>(label);
    text->set_width_percent(30);
    text->set_self_align(YGAlign::YGAlignCenter);

    control->set_width_percent(70);

    row->append(std::move(control));
}

} // namespace Slic3r::App
