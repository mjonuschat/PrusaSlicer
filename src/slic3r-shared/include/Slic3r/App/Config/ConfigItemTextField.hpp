///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"

namespace Slic3r::App {

class ConfigItemTextField : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::InputTextField {
public:

    ConfigItemTextField(size_t index, const Domain::ConfigItem& data);

protected:
    void on_data_update() override;
};

}
