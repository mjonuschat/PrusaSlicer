///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemSpinBox.hpp"

#include "Slic3r/App/Yoga/Validator.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemSpinBox::ConfigItemSpinBox(size_t index, const Domain::ConfigItem& data) :
    Biz::DataObserver<Domain::ConfigItem>(index, data),
    InputTextWithSpin(
        std::make_unique<IntValidator>(
            data.def().min.value_or(std::numeric_limits<int>::min()),
            data.def().max.value_or(std::numeric_limits<int>::max())
        )
    )
{
    on_data_update();
}

void ConfigItemSpinBox::on_data_update()
{
    set_text(fmt::format("{}", m_state->get<int>()));
}

} // namespace Slic3r::App
