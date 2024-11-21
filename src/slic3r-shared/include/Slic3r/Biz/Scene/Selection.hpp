#pragma once

#include <vector>
#include "Slic3r/Domain/ElementRef.hpp"

namespace Slic3r::Biz::Scene {

enum class SelectionMode
{
    Instance = 0,
    Volume
};

struct Selection
{
    using ElementRefs = std::vector<Domain::ElementRef>;

    SelectionMode mode{SelectionMode::Instance};
    ElementRefs elements;

    bool empty() const { return elements.empty(); }
};

}
