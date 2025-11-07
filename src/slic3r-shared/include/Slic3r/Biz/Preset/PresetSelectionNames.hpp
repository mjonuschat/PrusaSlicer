#pragma once

#include <string>
#include <vector>

namespace Slic3r::Biz::Preset {

struct PresetSelectionNames
{
    struct PresetName
    {
        std::string name;
        bool is_runtime_only;

        bool operator==(const PresetName& other) const
        {
            return this->name == other.name && this->is_runtime_only == other.is_runtime_only;
        }
    };

    PresetName printer;
    PresetName print;
    std::vector<PresetName> tools;
    std::vector<PresetName> materials;
};

} // namespace Slic3r::Biz::Preset