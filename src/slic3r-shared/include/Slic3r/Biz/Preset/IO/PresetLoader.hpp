#pragma once

#include "Slic3r/Domain/Preset/PresetTree.hpp"

#include <mutex>

namespace Slic3r::Biz::Preset::IO {

class PresetLoader
{
public:
    using PresetKind = Domain::Preset::PresetKind;
    using RootPresetNode = Domain::Preset::RootPresetNode;
    using PresetCollection = Domain::Preset::PresetCollection;
    using PresetOrigin = Domain::Preset::PresetOrigin;
    using PresetNamesCollection = Domain::Preset::PresetNamesCollection;

    void load(const std::string& file_name, std::mutex& mutex, PresetOrigin origin = PresetOrigin::System);
    void load_from_string(std::string_view source, PresetOrigin origin = PresetOrigin::System);
    void load_dir(const std::string& dir_path, PresetOrigin origin = PresetOrigin::System);
    const PresetCollection& presets() const { return m_presets; }
    std::tuple<PresetCollection, PresetNamesCollection> release();
private:
    void collect_names(const Domain::Preset::PresetNode& node, PresetKind kind, PresetOrigin origin);

private:
    using PresetNamesMap = std::map<std::string, Domain::Preset::PresetName>;
    using PresetNamesMapCollection = std::map<PresetKind, PresetNamesMap>;

    PresetCollection m_presets;
    PresetNamesMapCollection m_preset_names;
};

}
