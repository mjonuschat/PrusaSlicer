///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Yoga/InputTextWithSpin.hpp"

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App::Yoga {
class IntValidator;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemSpinBox : public ConfigItemControl, public Yoga::InputTextWithSpin
{
public:
    ConfigItemSpinBox(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::IConfigBoxSetter& cbi_container,
        size_t cbi_index
    );

    int value() const;

protected:
    void on_data_update() override;

    void update_value(const Domain::ConfigValue& value);

private:
    Biz::IConfigBoxSetter& m_cbi_container;
    size_t m_cbi_index{0};

    Yoga::IntValidator* m_value_validator{nullptr};
};

} // namespace Slic3r::App
