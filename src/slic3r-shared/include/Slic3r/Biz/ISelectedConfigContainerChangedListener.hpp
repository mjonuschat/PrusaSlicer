#pragma once

#include "SelectionId.hpp"

namespace Slic3r::Biz {
class ISelectedConfigContainerChangedListener
{
public:
    virtual ~ISelectedConfigContainerChangedListener() = default;

    virtual void on_selected_config_container_changed(SelectionId project_id, SelectionId container_id) = 0;
};
}
