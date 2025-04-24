#pragma once

#include "Slic3r/Domain/Preset/PresetTree.hpp"

namespace Slic3r::Biz::Preset::Loader {

class PresetLoader
{
public:
    using PresetKind = Domain::Preset::PresetKind;
    using RootPresetNode = Domain::Preset::RootPresetNode;
    using PresetCollection = Domain::Preset::PresetCollection;

    void load(const std::string& file_name);
    void load_from_string(std::string_view source);
    void load_dir(const std::string& dir_path);
    const PresetCollection& presets() const { return m_presets; }

private:
    PresetCollection m_presets;
};

}
