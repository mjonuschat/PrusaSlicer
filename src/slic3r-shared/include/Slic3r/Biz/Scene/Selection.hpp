#pragma once

#include <algorithm>
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
    bool is_selected(const Domain::ElementRef& ref) const
    {
        return std::any_of(elements.begin(), elements.end(), [ref](const Domain::ElementRef& r) {
            return ref.is_part_of(r);
        });
    }

    bool remove(const Domain::ElementRef& ref)
    {
        auto it = std::remove_if(elements.begin(), elements.end(), [ref](const auto& r) { return ref.is_part_of(r); });
        if (it != elements.end()) {
            elements.erase(it, elements.end());
            return true;
        }
        return false;
    }

    bool is_valid() const;
    void normalize();
};

}
