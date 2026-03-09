///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class ConfigItemTextField : public ConfigItemControl, public Yoga::InputTextField
{
public:
    ConfigItemTextField(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::IConfigBoxSetter& cbi_container,
        size_t cbi_index
    );

protected:
    void on_data_update() override;
    void update_value(const Domain::ConfigValue& value);

private:
    Biz::IConfigBoxSetter& m_cbi_container;
    size_t m_cbi_index{0};
    Yoga::Passthrough<Yoga::DoubleValidator> m_double_validator;
    Yoga::Passthrough<Yoga::PercentageValidator> m_percentage_validator;
    const Domain::ConfigItem* m_last_item{nullptr};
    std::optional<bool> m_is_multiline{std::nullopt};
    std::optional<bool> m_is_full_width{std::nullopt};
};

} // namespace Slic3r::App
