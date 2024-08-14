#pragma once

#include <vector>
#include "Slic3r/Domain/ElementId.hpp"

namespace Slic3r::Biz::Scene {

enum class SelectionMode
{
    Volume = 0,
    Instance
};

struct Selection
{
    using ElementIds = std::vector<Domain::ElementId>;

    SelectionMode mode;
    ElementIds elements;
};

}
