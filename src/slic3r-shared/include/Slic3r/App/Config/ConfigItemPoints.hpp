///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {
class InputTextField;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemPoints : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::Item
{
public:
    ConfigItemPoints(size_t index, const Domain::ConfigItem& data);

protected:
    void on_data_update() override;

private:
    Yoga::InputTextField* m_input_x{nullptr};
    Yoga::InputTextField* m_input_y{nullptr};
};

} // namespace Slic3r::App
