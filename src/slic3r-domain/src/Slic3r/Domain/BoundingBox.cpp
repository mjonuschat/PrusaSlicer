#include "Slic3r/Domain/BoundingBox.hpp"
#include <libassert/assert.hpp>

namespace Slic3r::Domain {

namespace Impl {
template<Domain::BoundingBoxConcept BoxType>
[[nodiscard]] bool is_equal(const BoxType& a, const BoxType& b) {
    ASSERT(a.defined && b.defined);
    return a.min == b.min && a.max == b.max;
}
}

[[nodiscard]] bool operator==(const BoundingBox2crd& a, const BoundingBox2crd& b) {
    return Impl::is_equal(a, b);
}

[[nodiscard]] bool operator==(const BoundingBox3crd& a, const BoundingBox3crd& b) {
    return Impl::is_equal(a, b);
}
}
