// src/boss/include/boss/foundation/BossFillRegistry.hpp
//
// Foundation template for the fill-dispatch extension family. Generated
// composition headers instantiate this with the traits of every feature
// that declares the "fill" capability, and also generate the shared
// boss_fill_pattern enum (BossFillPatternKey.hpp) from the same set --
// see the spec's corrected Section 3 for why this dispatch cannot live
// inside FillBase.cpp's native switch.
//
// create()/Features::create_fill() return std::unique_ptr<Fill>, matching
// the spec's own trait example exactly -- not a raw owning pointer.
//
// Fill is only forward-declared here, deliberately -- this header is
// #included transitively from slic3r-domain (via BossFills.hpp, from
// ConfigDefsFDM.cpp registering boss_fill_pattern), and slic3r-domain must
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
#include "Slic3r/Domain/ConfigDefUtils.hpp"
#include "boss/foundation/BossL.hpp"
#include "boss/foundation/BossRegistryCommon.hpp"
#include "boss/generated/BossFillPatternKey.hpp"

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
    // Lets callers outside this template (Fill.cpp's group_fills(), which
    // is not itself a template and so cannot use `if constexpr` against
    // the pack directly) check, without any runtime cost worth mentioning,
    // whether boss_fill_pattern was actually registered before reading
    // it. Reading a config key that was never added is not a graceful
    // "key not found" result in this codebase's ConfigView::get<T>() --
    // it's `ASSERT(it != m_values.end())` (Config.hpp), i.e. a crash in a
    // debug build and undefined behavior in release. See Fill.cpp's
    // group_fills() edit (this plan's Task 5, Step 5).
    static constexpr bool has_any_features = sizeof...(Features) > 0;

    // Registers the shared boss_fill_pattern option itself. No single
    // feature owns this option -- every fill-capable feature contributes
    // one enum choice to it, which is why this lives on the registry
    // rather than inside any one Features::register_config().
    //
    // Gated on a non-empty Features pack, deliberately: the spec requires
    // that an empty composition's "fixture output matches the selected
    // upstream baseline" -- a boss_fill_pattern option visible in Settings
    // with nothing to select but "None" would not match that baseline, it
    // would be a new, empty-but-present BOSS option upstream never had.
    // With zero fill features compiled in, this call is a true no-op, not
    // a partially-empty registration.
    static void register_config(Domain::ConfigDefinitions& defs)
    {
        if constexpr (sizeof...(Features) == 0) {
            return;
        }

        using namespace Slic3r::Domain;

        EnumValueDefs choices{{0, "none", BossL("None")}};
        (choices.push_back({Features::id, std::string(Features::key), BossL(std::string(Features::label))}), ...);
        // check_enum_def() (ConfigValue.hpp) asserts the list is sorted by
        // enum_value with no duplicates. The fold expression above appends
        // in template-parameter-pack order, which the generator emits
        // sorted by feature *name*, not by id -- sort explicitly here so a
        // future feature whose name sorts before an existing one but whose
        // id is numerically larger doesn't violate that assert. Duplicate
        // ids are already rejected at generation time (Task 1's
        // check_global_uniqueness), so std::sort alone (no separate
        // dedup pass) is sufficient here.
        std::sort(choices.begin(), choices.end());

        ConfigItemDef* def = defs.add("boss_fill_pattern", typeid(EnumWrapper));
        def->location      = FDMConfigLocation::Print;
        def->overrides_in  = std::set<ConfigLocation>{FDMConfigLocation::Tool, FDMConfigLocation::Object, FDMConfigLocation::Volume};
        def->category      = ConfigItemDef::Category::Print_Infill;
        def->option_group  = ConfigItemDef::OptionGroup::Print_Infill_DensityPattern;
        def->order         = 2;
        def->gui_type      = ConfigItemDef::GUIType::combobox;
        def->label         = BossL("BOSS fill pattern");
        def->tooltip       = BossL("Additional sparse infill patterns provided by BOSS. Overrides Fill pattern when set to a value other than None.");
        def->init_fn       = init_with(Domain::Boss::FillPatternKey::None, choices);
    }

    static std::optional<int> id_for_key(Domain::Boss::FillPatternKey key)
    {
        std::optional<int> result;
        ((static_cast<int>(key) == Features::id ? (result = Features::id, true) : false) || ...);
        return result;
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
