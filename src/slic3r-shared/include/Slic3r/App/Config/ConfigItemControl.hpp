///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Config.hpp"

#include "Slic3r/Biz/DataObserver.hpp"

#include <fmt/format.h>

namespace Slic3r::App {

class ConfigItemControl : public Biz::DataObserver<Domain::ConfigItem>
{
public:
    explicit ConfigItemControl(size_t index, const Domain::ConfigItem& data);

protected:
    std::optional<std::string> default_value() const;

    std::string tooltip_text() const;
};

} // namespace Slic3r::App
