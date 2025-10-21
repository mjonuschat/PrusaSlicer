#pragma once

#include <functional>
#include <vector>

#include "Slic3r/Domain/BedRef.hpp"
#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Domain/Workbench.hpp"

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

    [[nodiscard]] bool only_single_object() const;

    [[nodiscard]] bool is_valid() const;
    void normalize();
};

enum class BedSelectionMode
{
    SingleBed,
    ConfigContainer
};

struct BedSelection
{
    std::function<void(const BedSelection&)> on_change{[](const BedSelection&) {
    }};

    Domain::BedRef last_selected_bed() const;

    bool is_selected(const Domain::BedRef bed_ref) const;

    Domain::SelectionId config_container_id() const;

    bool empty() const;

    /** @brief Replace the selection with one. */
    bool select_one(const Domain::BedRef& bed_ref);

    /** @brief Add or remove from active selection. */
    bool toggle(const Domain::BedRef& bed_ref);

    bool set_mode(const BedSelectionMode mode);

    bool remove(const Domain::BedRef& bed_ref);

private:
    Domain::BedRefs m_selected_beds;
    Domain::SelectionId m_selected_config_container{Domain::INVALID_ID};
    Domain::BedRef m_last_selected_bed{Domain::INVALID_ID, Domain::INVALID_ID};
    BedSelectionMode m_mode{BedSelectionMode::SingleBed};
};

using BedInstanceRefWrap = std::reference_wrapper<const Domain::BedInstance>;
using BedInstances       = std::vector<BedInstanceRefWrap>;

[[nodiscard]] BedInstances get_selected_beds(
    const Domain::SelectionId project_id,
    const BedSelection& selection,
    const Domain::Workbench& workbench
);

} // namespace Slic3r::Biz::Scene
