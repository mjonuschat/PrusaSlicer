#pragma once

#include <functional>
#include <libslic3r/Model.hpp>
#include "Slic3r/Domain/BedInstance.hpp"

namespace Slic3r::Biz::Slicing {

void with_limited_instances(
    Model &model,
    const Domain::ModelInstanceList& bed_instances,
    const std::function<void()> &callable
);

}
