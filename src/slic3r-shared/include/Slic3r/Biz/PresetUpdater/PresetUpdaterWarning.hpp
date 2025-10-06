#pragma once

#include "fmt/format.h"
#include <string>

namespace Slic3r::Biz::PresetUpdater {

struct PresetUpdaterWarning
{
    std::string text;
    std::string repo;
    std::string vendor;
    
    std::string string() const
    {
        return fmt::format("{}/{}: {}",repo, vendor, text);
    }
};
}