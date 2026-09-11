#pragma once

#include <span>

#include "libslic3r/Point.hpp"

namespace Slic3r::Domain {
class ConfigView;
}

namespace Slic3r::Boss {

// Non-owning -- do not store a SeamVisibilityContext past the call that
// constructs it. visibility and normals are parallel arrays: both index the
// same mesh sample and must have the same length.
struct SeamVisibilityContext {
    std::span<float>       visibility;
    std::span<const Vec3f> normals;
    const Domain::ConfigView *config = nullptr;
};

} // namespace Slic3r::Boss
