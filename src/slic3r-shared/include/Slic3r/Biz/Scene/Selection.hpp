#pragma once

#include <vector>
#include "Slic3r/Domain/ElementRef.hpp"

namespace Slic3r::Biz::Scene {

enum class SelectionMode
{
    Volume = 0,
    Instance
};

struct Selection
{
    using ElementRefs = std::vector<Domain::ElementRef>;

    SelectionMode mode;
    ElementRefs elements;

    bool empty() const { return elements.empty(); }
};

}
