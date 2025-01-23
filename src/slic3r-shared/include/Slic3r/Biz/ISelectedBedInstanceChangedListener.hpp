#pragma once

#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::Biz {
class ISelectedBedInstanceChangedListener
{
public:
    virtual ~ISelectedBedInstanceChangedListener() = default;

    virtual void on_selected_bed_instance_changed(Domain::SelectionId project_id, Domain::SelectionId container_id, Domain::SelectionId bed_instance_id) = 0;
};
}
