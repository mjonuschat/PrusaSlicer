// src/boss/include/boss/foundation/BossFillRegistry.hpp
//
// Foundation template for the fill-dispatch extension family. Generated
// composition headers instantiate this with the traits of every feature that
// declares the "fill" capability. The dispatch cannot live in FillBase.cpp's
// native switch: the Fill subclasses live on the feature branches, so every
// fill feature would have to edit that one switch.
//
// create()/Features::create_fill() return std::unique_ptr<Fill>, matching
// the spec's own trait example exactly -- not a raw owning pointer.
//
// Fill is only forward-declared here, deliberately -- this header is
// #included transitively from slic3r-domain (via BossFills.hpp, from
// ConfigDefsFDM.cpp registering the fill choices), and slic3r-domain must
// not depend on libslic3r's FillBase.hpp (a higher layer than Domain, per
// the spec's Clean Architecture direction: App -> Biz -> Domain, never the
// reverse). This is safe: create()/use_bridge_flow()/anchoring_eligible()
// are members of a class TEMPLATE, so their bodies (the only places that
// actually need Fill complete) are not instantiated until something calls
// them -- which only ever happens from libslic3r's own Fill.cpp, which
// already includes FillBase.hpp fully. register_config(), the only member
// slic3r-domain actually calls, never touches Fill at all.
#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <string>

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"
#include "boss/foundation/BossL.hpp"
#include "boss/foundation/BossRegistryCommon.hpp"

namespace Slic3r {
class Fill;
}

namespace Slic3r::Boss {

template<class... Features>
struct BossFillRegistry {
private:
    static_assert(boss_ids_are_unique<Features...>(),
                  "duplicate BOSS feature id in this registry's composition");

public:
    // Lets non-template callers branch on whether any fill feature is
    // composed in without reaching into the pack themselves.
    static constexpr bool has_any_features = sizeof...(Features) > 0;

    // Every fill feature adds its pattern to the native fill_pattern option
    // rather than to a parallel one. Two independent pattern options left the
    // combination undefined -- grid here and crosshatch there had no answer.
    //
    // A feature's own id doubles as its InfillPattern value: ids are unique
    // across the composition already, and they sit above ipCount so
    // is_boss_pattern() can tell them from the native ones without a lookup.
    static void register_config(Domain::ConfigDefinitions& defs)
    {
        if constexpr (sizeof...(Features) == 0) {
            return;
        }

        (Domain::append_enum_choice(
             defs,
             "fill_pattern",
             Features::id,
             std::string(Features::key),
             BossL(std::string(Features::label))
         ),
         ...);
    }

    // Native patterns are the enumerators below ipCount; anything above is a
    // feature id this registry owns.
    static constexpr bool is_boss_pattern(const int value)
    {
        return value >= int(Domain::InfillPattern::ipCount);
    }


    static std::unique_ptr<Fill> create(int id)
    {
        std::unique_ptr<Fill> result;
        ((id == Features::id ? (result = Features::create_fill(), true) : false) || ...);
        return result;
    }

    static bool use_bridge_flow(int id)
    {
        bool result = false;
        ((id == Features::id ? (result = Features::use_bridge_flow(), true) : false) || ...);
        return result;
    }

    static bool anchoring_eligible(int id)
    {
        bool result = false;
        ((id == Features::id ? (result = Features::anchoring_eligible, true) : false) || ...);
        return result;
    }
};

} // namespace Slic3r::Boss
