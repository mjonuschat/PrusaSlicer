#pragma once

#include "libslic3r/Print.hpp"

namespace Slic3r::SlicingSync {

template <typename Set>
AllOrSome<Set> merge(const AllOrSome<Set>& a, const AllOrSome<Set>& b);

InvalidatedSteps merge(const InvalidatedSteps& a, const InvalidatedSteps& b);

InvalidatedSteps merge(const std::vector<InvalidatedSteps>& invalidated_steps);

PrintAndObjectSteps get_invalidated_steps(
    const PrintObjectRegions& current_regions,
    const PrintObjectRegions& next_regions
);

PrintAndObjectSteps diff_to_invalidated_steps(
    const StaticPrintConfig& old_config,
    const StaticPrintConfig& new_config,
    const std::vector<std::string>& diff
);

PrintAndObjectSteps diff_to_print_invalidated_steps(
    const std::vector<std::string>& option_keys
);


}
