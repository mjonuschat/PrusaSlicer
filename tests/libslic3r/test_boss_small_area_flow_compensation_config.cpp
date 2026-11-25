#include <algorithm>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"

using namespace Slic3r;

namespace {

const Domain::ConfigItemDef* find_option(const std::string& name)
{
    const Domain::ConfigDefinitions& defs = Domain::get_defs_fdm();
    auto it                               = std::find_if(
        defs.defs().begin(),
        defs.defs().end(),
        [&name](const Domain::ConfigItemDef& def) { return def.name == name; }
    );
    return it == defs.defs().end() ? nullptr : &*it;
}

} // namespace

TEST_CASE(
    "SmallAreaFlowCompensationFeature registers 21 options with correct bounds",
    "[boss][config]"
)
{
    const Domain::ConfigItemDef* enable = find_option("small_area_infill_flow_compensation");
    REQUIRE(enable != nullptr);

    for (int i = 0; i < 10; ++i) {
        INFO("point index: " << i);

        const Domain::ConfigItemDef* length = find_option(
            "small_area_infill_flow_compensation_extrusion_length_" + std::to_string(i)
        );
        REQUIRE(length != nullptr);
        REQUIRE(length->min.has_value());
        CHECK(*length->min == 0.0);
        REQUIRE(length->max.has_value());
        CHECK(*length->max == 100.0);

        const Domain::ConfigItemDef* factor = find_option(
            "small_area_infill_flow_compensation_compensation_factor_" + std::to_string(i)
        );
        REQUIRE(factor != nullptr);
        REQUIRE(factor->min.has_value());
        CHECK(*factor->min == 0.0);
        REQUIRE(factor->max.has_value());
        CHECK(*factor->max == 1.0);
    }
}

TEST_CASE(
    "SmallAreaFlowCompensationFeature default lengths and factors match the 2.9.x curve",
    "[boss][config]"
)
{
    const double default_lengths[10] = {0, 0.2, 0.4, 0.6, 0.8, 1.5, 2, 3, 5, 10};
    const double default_factors[10] =
        {0, 0.4444, 0.6145, 0.7059, 0.7619, 0.8571, 0.8889, 0.9231, 0.9520, 1.0};

    for (int i = 0; i < 10; ++i) {
        INFO("point index: " << i);

        const Domain::ConfigItemDef* length = find_option(
            "small_area_infill_flow_compensation_extrusion_length_" + std::to_string(i)
        );
        REQUIRE(length != nullptr);
        CHECK(length->init_fn().get<double>() == default_lengths[i]);

        const Domain::ConfigItemDef* factor = find_option(
            "small_area_infill_flow_compensation_compensation_factor_" + std::to_string(i)
        );
        REQUIRE(factor != nullptr);
        CHECK(factor->init_fn().get<double>() == default_factors[i]);
    }
}
