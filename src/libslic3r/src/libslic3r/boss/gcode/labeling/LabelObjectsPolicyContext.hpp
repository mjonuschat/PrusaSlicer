#pragma once

#include <string>
#include <vector>

namespace Slic3r { class Print; }

namespace Slic3r::Boss {

struct LabelObjectsDeclaration {
    std::string name;
    std::string center;
    std::string polygon;
};

// Read-only: the whole Print is needed to reach skirt/wipe-tower geometry
// (skirt_convex_hull(), first_layer_wipe_tower_corners()), so this context
// is a thin non-owning wrapper rather than a narrower value struct — revisit
// if a future consumer needs this hook without a full Print available.
struct LabelObjectsPolicyContext {
    const Print *print = nullptr;
};

} // namespace Slic3r::Boss
