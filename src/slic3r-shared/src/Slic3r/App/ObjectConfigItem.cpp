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
    Biz::IConfigBoxSetter& cbi_container
) :
    Biz::DataObserver<Biz::OverrideItem>(index, data),
    m_cbi_container(cbi_container)
{
    set_gap(5);

    m_label = emplace_back<Text>(std::string());

    on_data_update();
}

void ObjectConfigItem::on_data_update()
{
    ASSERT(!m_state->is_override());

    if (m_gui_type != m_state->config_item->def().gui_type) {
        m_gui_type = m_state->config_item->def().gui_type;

        if (m_control_item) {
            remove(m_control_item);
            m_control_item = nullptr;
            m_control      = nullptr;
        }

        m_control = ConfigItemControl::config_item_control_factory(
            this,
            1,
            m_index,
            *m_state->config_item,
            m_cbi_container,
	    0 // Object and Volume are always index 0
        );
        m_control_item = dynamic_cast<Item*>(m_control);
        ASSERT(m_control_item, "ConfigItem needs to derive from Yoga::Item");
    }

    m_label->set_text(m_state->config_item->def().label);

    m_control->set_state(*m_state->config_item);
    m_control->set_mixed(m_state->mixed);
}

} // namespace Slic3r::App
