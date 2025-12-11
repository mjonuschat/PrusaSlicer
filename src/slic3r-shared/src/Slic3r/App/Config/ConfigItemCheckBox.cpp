///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemCheckBox.hpp"

#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App {

ConfigItemCheckBox::ConfigItemCheckBox(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::IConfigBoxSetter& cbi_container,
    size_t cbi_index
) :
    ConfigItemControl(index, data),
    m_cbi_container(cbi_container),
    m_cbi_index(cbi_index)
{
    set_width(150);
    m_tooltip->set_text_wrap(true);
    m_tooltip->content_item()->set_width(350);
    set_tooltip(tooltip_text());

    callbacks().action = [this] {
        m_cbi_container.set_item_value(*m_state, Domain::ConfigValue{checked()}, m_cbi_index);
    };

    on_data_update();
}

void ConfigItemCheckBox::on_data_update()
{
    if (mixed()) {
        set_third_state(true);
        set_label(Biz::_u8L("Mixed"));
        set_font_type(Render::ImguiFontType::Italic);
        return;
    }

    set_label({});
    set_font_type(Render::ImguiFontType::Regular);
    if (!overriden().value_or(true)) {
        set_checked(m_cbi_container.get_override_original_value(*m_state, location_index())->get<bool>());
    } else {
        set_checked(m_state->value().get<bool>());
    }
}

} // namespace Slic3r::App
