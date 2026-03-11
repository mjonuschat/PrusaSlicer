#include "Slic3r/App/Yoga/PrinterSettingsButton.hpp"

#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App::Yoga {

PrinterSettingsButton::PrinterSettingsButton(const std::string& tooltip) : RectangleButton(tooltip)
{
    set_flex_shrink(0);
    set_allow_overlap(true);
    m_icon = emplace_back<Icon>(Render::Icon::None);
    m_icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
    m_icon->set_aspect_ratio(1);

    Item* texts_wrapper = emplace_back<Item>();
    texts_wrapper->set_orientation(Orientation::Vertical);
    texts_wrapper->set_flex_grow(1.f);

    m_printer_name = texts_wrapper->emplace_back<Text>("");
    m_printer_name->set_font_type(Render::ImguiFontType::Bold);
    m_printer_name->set_flex_grow(1.f);
    m_printer_name->set_wrap_mode(Text::WrapMode::WrapElide);

    m_preset_name = texts_wrapper->emplace_back<Text>("");
    m_preset_name->set_flex_grow(1.f);
    m_preset_name->set_wrap_mode(Text::WrapMode::WrapElide);

    auto add_button = [this](Render::Icon icon, const std::string& tooltip)
    {
        LayoutButton* button = emplace_back<LayoutButton>(std::string{}, icon, tooltip);
        button->set_self_align(YGAlignCenter);
        button->set_min_size({24.f, 24.f});
        button->set_flex_shrink(0);
        button->set_background_color(IM_COL32_BLACK_TRANS);
        // Extra button is hidden by default.
        // It can be shown in the settings dialog under certain conditions.
        button->set_visible(false);

        button->callbacks().hovered_changed = [this](bool) { update_btns_visibility(); };

        return button;
    };

    m_printers_btn =
        add_button(Render::Icon::ConfigContainer, Biz::_u8L("Show info about printer"));
    m_cog_btn = add_button(Render::Icon::PrintIconMarker, Biz::_u8L("Show extruder settings"));

    set_background_color(ImColor(41, 41, 41));
}

void PrinterSettingsButton::set_image(const std::string& image)
{
    m_icon->set_image(image);
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
    m_printers_btn->set_visible(is_visible);
    update_btns_visibility();
}

void PrinterSettingsButton::set_visible_cog(bool is_visible)
{
    m_cog_btn->set_visible(is_visible);
    update_btns_visibility();
}

std::function<void()>& PrinterSettingsButton::on_cog()
{
    return m_cog_btn->callbacks().action;
}

std::function<void()>& PrinterSettingsButton::on_printer()
{
    return m_printers_btn->callbacks().action;
}

void PrinterSettingsButton::checked_updated_internal()
{
    RectangleButton::checked_updated_internal();
    if (!checked()) {
        m_printers_btn->set_background_color(background_color());
    }
    update_btns_visibility();
}

void PrinterSettingsButton::hovered_updated_internal()
{
    RectangleButton::hovered_updated_internal();
    update_btns_visibility();
}

void PrinterSettingsButton::update_btns_visibility()
{
    auto tint_btn_icon = [this](LayoutButton* button)
    {
        if (button->is_visible()) {
            button->set_icon_tint(
                hovered() || button->hovered() ? ImColor(255, 255, 255) :
                                                 ImColor(IM_COL32_BLACK_TRANS)
            );
        }
    };

    tint_btn_icon(m_cog_btn);
    tint_btn_icon(m_printers_btn);
}

} // namespace Slic3r::App::Yoga
