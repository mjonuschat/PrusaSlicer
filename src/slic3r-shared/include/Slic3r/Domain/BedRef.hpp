#pragma once
#include <vector>

namespace Slic3r::Domain {

struct BedRef
{
    size_t bed_id{0};
    size_t bed_instance_id{0};
};

using BedRefs = std::vector<BedRef>;

}
