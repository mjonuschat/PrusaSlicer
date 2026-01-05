///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/IListObserver.hpp"

#include <Slic3r/Biz/Platform/ListenerScope.hpp>

#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Config/ConfigItemControl.hpp"

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class ConfigItemExtruderSelection :
    public ConfigItemControl,
    public Yoga::ComboBox,
    public Biz::IListObserver<Biz::Preset::ToolConfigItemObservableList>
{
public:
    ConfigItemExtruderSelection(
        size_t index,
        const Domain::ConfigItem& config_item,
        Biz::IConfigBoxSetter& cbi_container,
        size_t cbi_index
    );

protected:
    void on_inserted(const Biz::Preset::ToolConfigItemObservableList& data, size_t index) override;
    void on_removed(const Biz::IndexRange& index_range) override;
    void on_reset() override;
    void update_size();

    void on_data_update() override;
    void update_value(const Domain::ConfigValue& value);

private:
    Biz::ListenerScope<
        Biz::IListObserver<Biz::Preset::ToolConfigItemObservableList>,
        Biz::Preset::ToolConfigItemCompoundObservableList,
        ConfigItemExtruderSelection>
        m_tool_items_list_observer_scope;

    Biz::IConfigBoxSetter& m_cbi_container;
    size_t m_cbi_index = 0;
};

} // namespace Slic3r::App
