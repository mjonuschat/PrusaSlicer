#pragma once

#include <vector>

#include "Slic3r/Domain/BedRef.hpp"
#include "Slic3r/Domain/ElementRef.hpp"

namespace Slic3r::Biz::Scene {

enum class SelectionMode
{
    Instance = 0,
    Volume
};

struct ObjectSelection
{
    using ElementRefs = std::vector<Domain::ElementRef>;

    SelectionMode mode{SelectionMode::Instance};
    ElementRefs elements;

    [[nodiscard]] bool empty() const;
    [[nodiscard]] bool is_selected(const Domain::ElementRef& ref) const;

    bool remove(const Domain::ElementRef& ref);

    [[nodiscard]] bool is_valid() const;
    void normalize();
};

struct BedSelection
{
    Domain::BedRef last_selected_bed() const;

    Domain::BedRefs all() const;

    bool is_selected(const Domain::BedRef bed_ref) const;

    bool empty() const;

    bool select_one(const Domain::BedRef& bed_ref);

    bool toggle(const Domain::BedRef& bed_ref);

    bool remove(const Domain::BedRef& bed_ref);

private:
    Domain::BedRefs m_selected_beds;
};

} // namespace Slic3r::Biz::Scene
