///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/ObjectConfigItem.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Config/ConfigItemControl.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ObjectConfigItem::ObjectConfigItem(
    size_t index,
    const Biz::OverrideItem& data,
    Biz::Preset::PresetInteractor& preset_interactor
) :
    Biz::DataObserver<Biz::OverrideItem>(index, data),
    m_preset_interactor(preset_interactor)
{
    set_gap(5);

    m_label = emplace_back<Text>(std::string());

    m_control = ConfigItemControl::config_item_control_factory(
        this,
        index,
        *data.config_item,
        m_preset_interactor
    );
    m_control_item = dynamic_cast<Item*>(m_control);
    ASSERT(m_control_item, "ConfigItem needs to derive from Yoga::Item");

    on_data_update();
}

void ObjectConfigItem::on_data_update()
{
    ASSERT(!m_state->is_override());

    m_label->set_text(m_state->config_item->def().label);

    m_control->set_state(*m_state->config_item);
    m_control->set_mixed(m_state->mixed);
}

} // namespace Slic3r::App
