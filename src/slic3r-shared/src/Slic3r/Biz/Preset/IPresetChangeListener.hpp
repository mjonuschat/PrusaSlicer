#pragma once


#include "Slic3r/Biz/SelectionId.hpp"
#include <libslic3r/Preset.hpp>

namespace Slic3r::Biz::Preset {

struct PresetState;

class IPresetChangeListener
{
public:
    virtual ~IPresetChangeListener() = default;

    virtual void on_bed_preset_changed(Slic3r::Preset::Type preset_type, PresetState& preset) = 0;
    virtual void on_object_preset_changed(Slic3r::Preset::Type preset_type, PresetState& preset) = 0;
};

}
