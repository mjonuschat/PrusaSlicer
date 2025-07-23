///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {
class InputTextField;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemTextFields : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::Item
{
public:
    ConfigItemTextFields(size_t index, const Domain::ConfigItem& data);

protected:
    void on_data_update() override;

private:
    std::vector<Yoga::InputTextField*> m_fields;
};

} // namespace Slic3r::App
