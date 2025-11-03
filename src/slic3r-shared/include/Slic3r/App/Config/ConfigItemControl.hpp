///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Config.hpp"

#include "Slic3r/Biz/DataObserver.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::Yoga {
class Item;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemControl : public Biz::DataObserver<Domain::ConfigItem>
{
public:
    explicit ConfigItemControl(size_t index, const Domain::ConfigItem& data);

    static ConfigItemControl* config_item_control_factory(
        Yoga::Item* container,
        size_t child_index,
        size_t data_index,
        const Domain::ConfigItem& item,
        Biz::Preset::PresetInteractor& preset_interactor,
        size_t cbi_index
    );

    bool mixed() const;
    void set_mixed(bool mixed);

    std::optional<bool> overriden() const;
    void set_overriden(std::optional<bool> overriden);

    int location_index() const;
    void set_location_index(int location_index);

protected:
    std::optional<std::string> default_value() const;

    std::string tooltip_text() const;

private:
    bool m_mixed         = false;
    int m_location_index = 0;
    std::optional<bool> m_overriden;
};

} // namespace Slic3r::App
