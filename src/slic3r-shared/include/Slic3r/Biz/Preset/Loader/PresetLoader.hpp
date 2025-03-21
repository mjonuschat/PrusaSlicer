#pragma once

#include "Slic3r/Domain/Preset/PresetTree.hpp"

namespace Slic3r::Biz::Preset::Loader {

class PresetLoader
{
public:
    using PresetKind = Domain::Preset::PresetKind;
    using RootPresetNode = Domain::Preset::RootPresetNode;
    using PresetCollection = Domain::Preset::EnumCollection<PresetKind, RootPresetNode>;

    void load(const std::string & file_name);
    const PresetCollection& presets() const { return m_presets; }

private:
    PresetCollection m_presets;
};

}
