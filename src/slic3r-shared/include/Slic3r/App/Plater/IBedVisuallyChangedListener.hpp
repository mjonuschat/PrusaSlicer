#pragma once

#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Domain/BedRef.hpp"

namespace Slic3r::App::Plater {

class IBedVisuallyChangedListener
{
public:
    virtual ~IBedVisuallyChangedListener() = default;

    virtual void on_bed_changed(Domain::SelectionId project_id, const Domain::BedRefs& bed_refs) = 0;
};

} // namespace Slic3r::App::Plater
