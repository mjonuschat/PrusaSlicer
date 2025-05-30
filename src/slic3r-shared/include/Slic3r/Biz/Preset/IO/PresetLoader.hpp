#pragma once

#include "Slic3r/Domain/Preset/PresetTree.hpp"

namespace Slic3r::Biz::Preset::IO {

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
    PresetCollection release()
    {
        PresetCollection ret = std::move(m_presets);
        m_presets.clear();
        return ret;
    }

private:
    PresetCollection m_presets;
};

}
