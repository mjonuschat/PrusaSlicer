#include "Slic3r/Biz/Scene/Selection.hpp"
#include <unordered_set>

namespace Slic3r::Biz::Scene {

bool Selection::is_valid() const
{
    const bool require_zero_vol_id = mode == SelectionMode::Instance;
    return std::all_of(elements.begin(), elements.end(), [require_zero_vol_id](const auto& e) {
        return require_zero_vol_id == (e.volume_id == 0);
    });
}

void Selection::normalize()
{
    mode = SelectionMode::Volume;
    if (elements.empty()) {
        return;
    }

    // verify if promoting to Instance mode is needed
    const auto inst_id = elements.front().instance_id;
    const bool requires_instance_mode = std::any_of(elements.begin(), elements.end(), [inst_id](const auto& e) {
        return e.volume_id == 0 || e.instance_id != inst_id;
    });

    if (requires_instance_mode) {
        mode = SelectionMode::Instance;
        std::unordered_set<Domain::ElementRef> unique_inst_elements;

        for (const auto& e : elements)
            unique_inst_elements.insert(Domain::ElementRef{e.object_id, e.instance_id, 0});

        elements.clear();
        elements.insert(elements.end(), unique_inst_elements.begin(), unique_inst_elements.end());
    }
}

} // namespace Slic3r::Biz::Scene
