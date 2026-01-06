///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App::Yoga {
class InputTextField;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemPoints : public ConfigItemControl, public Yoga::Item
{
public:
    ConfigItemPoints(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::IConfigBoxSetter& cbi_container,
        size_t cbi_index
    );

protected:
    void on_data_update() override;
    void send_data();
    void construct_points();

private:
    struct Point
    {
        Yoga::Item* container{nullptr};
        Yoga::InputTextField* input_x{nullptr};
        Yoga::InputTextField* input_y{nullptr};
        Yoga::Passthrough<Yoga::DoubleValidator> validator_x;
        Yoga::Passthrough<Yoga::DoubleValidator> validator_y;
    };

    Biz::IConfigBoxSetter& m_cbi_container;
    size_t m_cbi_index{0};

    std::vector<Point> m_points;
};

} // namespace Slic3r::App
