#pragma once

#include <map>
#include <string>

#include "libslic3r/Print.hpp"

namespace Slic3r::SlicingSync {

template <typename Set>
AllOrSome<Set> merge(const AllOrSome<Set>& a, const AllOrSome<Set>& b);

InvalidatedSteps merge(const InvalidatedSteps& a, const InvalidatedSteps& b);

InvalidatedSteps merge(const std::vector<InvalidatedSteps>& invalidated_steps);

using Step = std::variant<FDMPrintStep, FDMPrintObjectStep>;

std::vector<Step> propagate(Step step);

std::vector<Step> steps(const std::vector<std::vector<Step>>& steps);

std::map<std::string, std::vector<Step>> boss_step_invalidations();

std::map<std::string, std::vector<Step>> merge_boss_invalidations(
    std::map<std::string, std::vector<Step>> base,
    const std::map<std::string, std::vector<Step>>& boss
);

const std::map<std::string, std::vector<Step>>& invalidated_steps_table();

PrintAndObjectSteps get_invalidated_steps(
    const PrintObjectRegions& current_regions,
    const PrintObjectRegions& next_regions
);
}
