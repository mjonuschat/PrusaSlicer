#pragma once

#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::Biz::Preset {

class IPresetChangedListener
{
public:
    virtual ~IPresetChangedListener() = default;

    virtual void on_preset_selection_changed(Domain::SelectionId project_id) {}

    virtual void on_preset_value_changed(Domain::SelectionId project_id) {}

    virtual void on_config_container_selection_changed(Domain::SelectionId project_id) {}
};

} // namespace Slic3r::Biz::Preset
