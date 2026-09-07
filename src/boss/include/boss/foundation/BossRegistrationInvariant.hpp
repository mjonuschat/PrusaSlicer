///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <algorithm>
#include <iterator>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace Slic3r::Boss {

namespace detail {

inline std::string join(const std::vector<std::string>& keys)
{
    return std::accumulate(
        std::next(keys.begin()), keys.end(), keys.front(),
        [](std::string acc, const std::string& key) { return acc + ", " + key; }
    );
}

} // namespace detail

// Throws std::logic_error if the BOSS config keys the registries registered
// (registered) differ from the keys the manifests declare (declared).
inline void assert_boss_registration_matches_manifest(
    const std::set<std::string>& registered,
    const std::set<std::string>& declared
)
{
    std::vector<std::string> only_registered;
    std::set_difference(
        registered.begin(), registered.end(), declared.begin(), declared.end(),
        std::back_inserter(only_registered)
    );
    if (!only_registered.empty()) {
        throw std::logic_error(
            "BOSS config key(s) registered with no manifest config_options entry: "
            + detail::join(only_registered)
        );
    }

    std::vector<std::string> only_declared;
    std::set_difference(
        declared.begin(), declared.end(), registered.begin(), registered.end(),
        std::back_inserter(only_declared)
    );
    if (!only_declared.empty()) {
        throw std::logic_error(
            "BOSS config key(s) declared in a manifest but never registered: "
            + detail::join(only_declared)
        );
    }
}

} // namespace Slic3r::Boss
