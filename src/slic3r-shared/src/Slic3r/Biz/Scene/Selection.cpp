#include <algorithm>
#include "Slic3r/Biz/Scene/Selection.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::Biz::Scene {

SelectionState ObjectSelection::state() const {
    using Domain::ElementRef;

    if (elements.empty()) {
        return SelectionState::Empty;
    }

    const bool single_instance{std::ranges::all_of(
        elements,
        [&](const ElementRef& element)
        {
            if (!element.has_instance()) {
                return false;
            }
            return element.instance_id == elements.front().instance_id;
        }
    )};

    const bool single_volume{elements.size() == 1 && elements.front().has_volume()};

    if (single_volume) {
        return SelectionState::SingleVolume;
    } else if (single_instance && mode == SelectionMode::Instance) {
        return SelectionState::WholeInstance;
    } else if (single_instance && mode == SelectionMode::Volume){
        return SelectionState::MultipleVolumes;
    } else {
        ASSERT(!single_instance);
        return SelectionState::MultipleInstances;
    }
}

bool ObjectSelection::empty() const
{
    return elements.empty();
}

bool ObjectSelection::is_selected(const Domain::ElementRef& ref) const
{
    return std::any_of(elements.begin(), elements.end(), [ref](const Domain::ElementRef& r) {
        return ref.is_part_of(r);
    });
}

bool ObjectSelection::only_single_object() const
{
    const size_t n = elements.size();
    if (contains_wipe_tower()) {
        return false;
    }
    if (n == 0)
        return false;
    if (n == 1)
        return true;
    auto it = elements.cbegin();
    const auto obj_id = it->object_id;
    return std::all_of(
        ++it,
        elements.cend(),
        [obj_id](const Domain::ElementRef& r) { return r.object_id == obj_id; }
    );
}

bool ObjectSelection::remove(const Domain::ElementRef& ref)
{
    auto it = std::remove_if(elements.begin(), elements.end(), [ref](const auto& r) {
        return ref.is_part_of(r);
    });
    if (it != elements.end()) {
        elements.erase(it, elements.end());
        return true;
    }
    return false;
}

bool ObjectSelection::contains_wipe_tower() const
{
    return std::ranges::any_of(
        elements,
        [](const Domain::ElementRef& element) { return element.is_wipe_tower(); }
    );
}

bool ObjectSelection::is_valid() const
{
    const bool require_zero_vol_id = mode == SelectionMode::Instance;
    return std::all_of(elements.begin(), elements.end(), [require_zero_vol_id](const Domain::ElementRef& e) {
        if (e.is_wipe_tower()) {
            return true;
        }
        return require_zero_vol_id == (e.volume_id == 0) && e.instance_id != 0;
    });
}

Domain::BedRef BedSelection::last_selected_bed() const
{
    ASSERT(
        m_last_selected_bed.config_container_id != Domain::INVALID_ID
        && m_last_selected_bed.instance_id != Domain::INVALID_ID
    );
    return m_last_selected_bed;
}

bool BedSelection::is_selected(const Domain::BedRef bed_ref) const
{
    if (m_mode == BedSelectionMode::SingleBed) {
        ASSERT(!m_selected_beds.empty());
        const auto it{std::ranges::find(m_selected_beds, bed_ref)};
        return it != m_selected_beds.end();
    } else if (m_mode == BedSelectionMode::ConfigContainer) {
        ASSERT(m_selected_config_container != Domain::INVALID_ID);
        return bed_ref.config_container_id == m_selected_config_container;
    }
    PANIC("Unknown mode!");
}

Domain::SelectionId BedSelection::config_container_id() const
{
    if (m_mode == BedSelectionMode::SingleBed) {
        ASSERT(!m_selected_beds.empty());
        return m_selected_beds.front().config_container_id;
    } else if (m_mode == BedSelectionMode::ConfigContainer) {
        return m_selected_config_container;
    }
    PANIC("Unknown mode!");
}

bool BedSelection::empty() const
{
    return m_selected_beds.empty();
}

bool BedSelection::select_one(const Domain::BedRef& bed_ref, CameraActionOnBedSelection camera_action)
{
    m_camera_action_on_selection = camera_action;

    if (m_mode == BedSelectionMode::SingleBed) {
        if (m_selected_beds.size() == 1 && m_selected_beds.front() == bed_ref &&
            camera_action != CameraActionOnBedSelection::CenterOnBed) {
            return false;
        }
        m_selected_beds     = {bed_ref};
        m_last_selected_bed = bed_ref;
        on_change(*this);
        return true;
    } else if (m_mode == BedSelectionMode::ConfigContainer) {
        ASSERT(bed_ref.config_container_id != Domain::INVALID_ID);
        if (m_selected_config_container == bed_ref.config_container_id
            && m_last_selected_bed == bed_ref)
        {
            return false;
        }
        m_selected_config_container = bed_ref.config_container_id;
        m_selected_beds = {bed_ref};
        m_last_selected_bed = bed_ref;
        on_change(*this);
        return true;
    }
    PANIC("Unknown mode!");
}

bool BedSelection::toggle(const Domain::BedRef& bed_ref, CameraActionOnBedSelection camera_action)
{
    m_camera_action_on_selection = camera_action;

    if (m_mode == BedSelectionMode::ConfigContainer) {
        return false;
    }

    if (m_selected_beds.size() == 1 && m_selected_beds.front() == bed_ref) {
        return false;
    }
    if (remove(bed_ref)) {
        on_change(*this);
        return true;
    }
    m_selected_beds.push_back(bed_ref);
    m_last_selected_bed = bed_ref;
    on_change(*this);
    return true;
}

bool BedSelection::set_mode(const BedSelectionMode mode)
{
    if (m_mode == mode) {
        return false;
    }

    m_mode = mode;
    if (m_mode == BedSelectionMode::ConfigContainer) {
        m_selected_config_container = last_selected_bed().config_container_id;
    } else if (m_mode == BedSelectionMode::SingleBed) {
        m_selected_config_container = Domain::INVALID_ID;
        const auto it{std::ranges::find(m_selected_beds, m_last_selected_bed)};
        if (it == m_selected_beds.end() && !m_selected_beds.empty()) {
            m_last_selected_bed = m_selected_beds.back();
        }
    }

    on_change(*this);
    return true;
}

bool BedSelection::remove(const Domain::BedRef& bed_ref)
{
    const std::size_t erased_count{std::erase(m_selected_beds, bed_ref)};
    if (m_last_selected_bed == bed_ref) {
        ASSERT(erased_count == 1);
        if (!m_selected_beds.empty()) {
            m_last_selected_bed = m_selected_beds.back();
        } else {
            m_last_selected_bed = {Domain::INVALID_ID, Domain::INVALID_ID};
        }
    }

    return erased_count != 0;
}

void BedSelection::set_state(
    Domain::BedRefs selected_beds,
    Domain::SelectionId selected_config_container,
    Domain::BedRef last_selected_bed,
    BedSelectionMode mode,
    CameraActionOnBedSelection camera_action_on_selection
)
{
    m_selected_beds = std::move(selected_beds);
    m_selected_config_container = selected_config_container;
    m_last_selected_bed = last_selected_bed;
    m_mode  = mode;
    m_camera_action_on_selection = camera_action_on_selection;
    on_change(*this);
}

BedInstances get_selected_beds(
    const Domain::SelectionId project_id,
    const BedSelection& selection,
    const Domain::Workbench& workbench
)
{
    BedInstances result;

    const Domain::ConfigContainer* config_container{
        workbench.project(project_id).find_config_container(selection.config_container_id())
    };

    if (config_container == nullptr) {
        return {};
    }

    for (const auto& bed_instance : config_container->bed_instances()) {
        if (selection.is_selected(Domain::BedRef{config_container->id().id, bed_instance->id().id}))
            result.push_back(*bed_instance);
    }

    std::ranges::sort(result, [](const BedInstanceRefWrap& a, const BedInstanceRefWrap& b) {
        return a.get().index() < b.get().index();
    });

    return result;
}

} // namespace Slic3r::Biz::Scene
