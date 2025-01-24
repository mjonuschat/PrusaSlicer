#pragma once
#include <vector>

namespace Slic3r::Domain {

struct BedRef
{
    size_t config_container_id{0};
    size_t instance_id{0};
};

inline bool operator == (const BedRef& br1, const BedRef& br2)
{
    return
        br1.config_container_id == br2.config_container_id &&
        br1.instance_id == br2.instance_id;
}

using BedRefs = std::vector<BedRef>;

}
