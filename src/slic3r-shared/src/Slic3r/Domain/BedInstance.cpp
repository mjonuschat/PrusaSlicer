#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::Domain {

BedInstance::BedInstance(const Bed& bed): m_bed(bed) { DEBUG_ASSERT(this->id().valid()); }

} // namespace Slic3r::Domain
