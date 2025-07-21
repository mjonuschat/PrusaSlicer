#include <unordered_set>
#include <algorithm>
#include "Slic3r/Biz/Scene/Selection.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::Biz::Scene {

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

bool ObjectSelection::is_valid() const
{
    const bool require_zero_vol_id = mode == SelectionMode::Instance;
    return std::all_of(elements.begin(), elements.end(), [require_zero_vol_id](const auto& e) {
        return require_zero_vol_id == (e.volume_id == 0) && e.instance_id != 0;
    });
}

void ObjectSelection::normalize()
{
    mode = SelectionMode::Volume;
    if (elements.empty()) {
        return;
    }

    // verify if promoting to Instance mode is needed
    const auto inst_id                = elements.front().instance_id;
    const bool requires_instance_mode = std::any_of(
        elements.begin(),
        elements.end(),
        [inst_id](const auto& e) { return e.volume_id == 0 || e.instance_id != inst_id; }
    );

    if (requires_instance_mode) {
        mode = SelectionMode::Instance;
        std::unordered_set<Domain::ElementRef> unique_inst_elements;

        for (const auto& e : elements)
            unique_inst_elements.insert(Domain::ElementRef{e.object_id, e.instance_id, 0});

        elements.clear();
        elements.insert(elements.end(), unique_inst_elements.begin(), unique_inst_elements.end());
    }
}

Domain::BedRef BedSelection::last_selected_bed() const
{
    ASSERT(!m_selected_beds.empty(), "No selected beds! Did you forget calling add()?");
    return m_selected_beds.back();
}

Domain::BedRefs BedSelection::all() const
{
    ASSERT(!m_selected_beds.empty());
    return m_selected_beds;
}

bool BedSelection::is_selected(const Domain::BedRef bed_ref) const
{
    ASSERT(!m_selected_beds.empty());
    const auto it{std::ranges::find(m_selected_beds, bed_ref)};
    return it != m_selected_beds.end();
}

bool BedSelection::empty() const {
    return m_selected_beds.empty();
}

bool BedSelection::select_one(const Domain::BedRef& bed_ref)
{
    if (m_selected_beds.size() == 1 && m_selected_beds.front() == bed_ref) {
        return false;
    }
    m_selected_beds = {bed_ref};
    return true;
}

bool BedSelection::toggle(const Domain::BedRef& bed_ref)
{
    if (m_selected_beds.size() == 1 && m_selected_beds.front() == bed_ref) {
        return false;
    }
    if (remove(bed_ref)) {
        return true;
    }
    m_selected_beds.push_back(bed_ref);
    return true;
}

bool BedSelection::remove(const Domain::BedRef& bed_ref)
{
    if (m_selected_beds.size() <= 1) {
        return false;
    }
    return std::erase(m_selected_beds, bed_ref) != 0;
}

} // namespace Slic3r::Biz::Scene
