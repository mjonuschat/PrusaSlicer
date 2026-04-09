///|/ Copyright (c) Prusa Research 2026 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemRammingParams.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/IDialogManager.hpp"

#include "Slic3r/Biz/IConfigBoxSetter.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemRammingParams::ConfigItemRammingParams(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::IConfigBoxSetter& cbi_container,
    size_t cbi_index
) :
    ConfigItemControl(index, data),
    LayoutButton(Biz::_u8L("Ramming settings") + "..."),
    m_cbi_container(cbi_container),
    m_cbi_index(cbi_index)
{
    set_background_color(Platform::Color::Button);
    m_tooltip->set_text_wrap(true);
    m_tooltip->content_item()->set_width(350);
    set_tooltip(tooltip_text());

    on_data_update();

    callbacks().action = [this]()
    {
        const std::string init_val = m_state->get<std::string>();
        const std::string ret =
            App::AppServices::instance().dialog_manager().show_ramming_dialog(init_val);
        if (ret != init_val) {
            m_cbi_container.set_item_value(*m_state, Domain::ConfigValue{ret}, m_cbi_index);
        }
    };
}

void ConfigItemRammingParams::on_data_update() {}

} // namespace Slic3r::App
