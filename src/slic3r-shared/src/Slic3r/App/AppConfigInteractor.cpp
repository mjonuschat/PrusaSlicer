///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/AppConfigInteractor.hpp"

namespace Slic3r::App {

AppConfigInteractor::AppConfigInteractor(Domain::ConfigBox* app_config_box)
{
    m_app_config_cbi = Biz::ConfigBoxInteractor(m_cbi_accessor, nullptr);
    m_app_config_cbi.set_config_box(app_config_box);
}

Biz::ConfigBoxInteractor& AppConfigInteractor::app_config_cbi()
{
    return m_app_config_cbi;
}

const Domain::ConfigValue*
AppConfigInteractor::get_override_original_value(const Domain::ConfigItem& item, size_t index) const
{
    return m_app_config_cbi.find(item.name());
}

void AppConfigInteractor::set_item_value(
    const Domain::ConfigItem& item,
    const Domain::ConfigValue& value,
    size_t index
)
{
    m_cbi_accessor.set_value(item.name(), value);

    invoke_listeners<IAppConfigChangedListener>([](auto* l) { l->on_app_config_changed(); });
}

} // namespace Slic3r::App
