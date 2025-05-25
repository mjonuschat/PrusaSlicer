#include "Slic3r/App/Yoga/PrinterSettingsButton.hpp"

#include "Slic3r/App/Yoga/Text.hpp"

namespace Slic3r::App::Yoga {

PrinterSettingsButton::PrinterSettingsButton(const std::string& tooltip)
    : LayoutButton("", Render::Icon::None, tooltip)
{
    set_checkable(true);

    size_t index = 1; // index of the hidden button default label
    Item* texts_wrapper = insert_into_content(std::make_unique<Item>(), ++index);
    texts_wrapper->set_orientation(Orientation::Vertical);
    texts_wrapper->set_flex_grow(1.f);
    texts_wrapper->set_height_percent(80);
    texts_wrapper->set_justify_content(YGJustifySpaceBetween);
    m_printer_name = texts_wrapper->emplace_back<Text>("");
    m_printer_name->set_font_type(Render::ImguiFontType::Bold);
    m_preset_name = texts_wrapper->emplace_back<Text>("");

    Vec2f btn_sz{20.f, 20.f};

    m_printers_btn = static_cast<LayoutButton*>(insert_into_content(
        std::make_unique<LayoutButton>("", Render::Icon::ConfigContainer, "Show info about printer"),
        ++index
    ));
    m_printers_btn->set_self_align(YGAlignCenter);
    m_printers_btn->set_max_size(btn_sz);

    m_cog_btn = static_cast<LayoutButton*>(insert_into_content(
        std::make_unique<LayoutButton>("", Render::Icon::PrintIconMarker, "Show extruder settings"),
        ++index
    ));
    m_cog_btn->set_self_align(YGAlignCenter);
    m_cog_btn->set_max_size(btn_sz);

    // Extra button is hidden by default.
    // It can be shown in the settings dialog under certain conditions.
    m_cog_btn->set_visible(false);
    m_printers_btn->set_visible(false);

    set_background_color(ImColor(41, 41, 41));
}

void PrinterSettingsButton::set_printer_name(const std::string& printer_name)
{
    m_printer_name->set_text(printer_name);
}

void PrinterSettingsButton::set_preset_name(const std::string& preset_name)
{
    m_preset_name->set_text(preset_name);
}

void PrinterSettingsButton::set_printing_state(int state)
{
    // ToDo render colored circle as a printing state with Tooltip
}

void PrinterSettingsButton::set_visible_printer(bool is_visible)
{
    if (m_is_visible_printers != is_visible) {
        m_is_visible_printers = is_visible;
        m_printers_btn->set_background_color(background_color());
        update_btns_visibility();
    }
}

void PrinterSettingsButton::set_visible_cog(bool is_visible)
{
    if (m_is_visible_cog != is_visible) {
        m_is_visible_cog = is_visible;
        m_cog_btn->set_background_color(background_color());
        update_btns_visibility();
    }
}

std::function<void()>& PrinterSettingsButton::on_cog() { return m_cog_btn->callbacks().action; }

std::function<void()>& PrinterSettingsButton::on_printer()
{
    return m_printers_btn->callbacks().action;
}

void PrinterSettingsButton::checked_updated_internal()
{
    update_btns_visibility();
    LayoutButton::checked_updated_internal();

    if (!checked()) {
        m_cog_btn->set_background_color(background_color());
        m_printers_btn->set_background_color(background_color());
    }
}

void PrinterSettingsButton::hovered_updated_internal()
{
    update_btns_visibility();
    LayoutButton::hovered_updated_internal();
}

void PrinterSettingsButton::update_btns_visibility()
{
    bool is_visible_btns = !checked() && hovered();
    m_printers_btn->set_visible(m_is_visible_printers && is_visible_btns);
    m_cog_btn->set_visible(m_is_visible_cog && is_visible_btns);
}

} // namespace Slic3r::App::Yoga
