#pragma once

#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::Biz {

class ISlicingInputChangedListener {
public:
    virtual ~ISlicingInputChangedListener() = default;
    virtual void on_slicing_input_changed(Domain::SelectionId bed_instance_id) = 0;
};

}
