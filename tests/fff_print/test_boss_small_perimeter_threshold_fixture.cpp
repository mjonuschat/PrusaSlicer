#include <catch2/catch_test_macros.hpp>

#include <set>

#include "libslic3r/libslic3r.h"
#include "Slic3r/Biz/GCodeReader/GCodeReader.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using Biz::GCodeReader::GCodeReader;
using Test::TestConfig;
using Domain::FloatOrPercentage;
using Domain::Percentage;

// tests/data/boss/small-perimeter-threshold/config.ini equivalent: the settings
// below reproduce the BOSS small-perimeter-threshold fixture config, applied
// directly to TestConfig rather than loaded from an .ini file (matching the
// established pattern in test_boss_perimeter_overlap_fixture.cpp).
SCENARIO("BOSS small-perimeter-threshold blends speed by perimeter length", "[boss][perimeter]")
{
    GIVEN("a cube with a small square hole, with distinct perimeter/small-perimeter speeds") {
        TestConfig config{1};
        config.print.items.opt("skirts").set(0);
        config.print.items.opt("perimeters").set(1);
        config.print.items.opt("layer_height").set(0.2);
        config.print.items.opt("perimeter_speed").set(99.0);
        config.print.items.opt("external_perimeter_speed").set(FloatOrPercentage{99.0});
        config.print.items.opt("small_perimeter_speed").set(FloatOrPercentage{11.0});
        config.print.items.opt("small_perimeter_min_length").set(45.0);
        config.print.items.opt("small_perimeter_max_length").set(125.0);
        config.print.items.opt("thin_walls").set(false);
        config.filament.at(0).items.opt("cooling").set(false);
        config.print.items.opt("first_layer_speed").set(FloatOrPercentage{Percentage{100}});

        WHEN("sliced") {
            const std::string gcode = Slic3r::Test::slice({Test::TestMesh::cube_with_hole}, config);

            std::set<double> feedrates;
            GCodeReader       parser;
            parser.parse_buffer(gcode, [&feedrates](GCodeReader &self, const GCodeReader::GCodeLine &line) {
                if (line.extruding(self) && line.dist_XY(self) > 0)
                    feedrates.insert(line.new_F(self));
            });

            const double perimeter_speed       = config.print.items.opt("perimeter_speed").get<double>() * 60.;
            const double small_perimeter_speed = config.print.items.opt("small_perimeter_speed")
                                                      .get<FloatOrPercentage>()
                                                      .get_abs_value(perimeter_speed / 60.) * 60.;

            THEN("the small hole loop and the large outer loop are extruded at different feedrates") {
                REQUIRE(feedrates.size() > 1);
            }
            THEN("the small hole loop's feedrate is at or below small_perimeter_speed") {
                REQUIRE(*feedrates.begin() <= small_perimeter_speed + 1.);
            }
            THEN("no feedrate exceeds the plain perimeter speed") {
                REQUIRE(*feedrates.rbegin() <= perimeter_speed + 1.);
            }
        }
    }

    GIVEN("an overhang model with an aggressive small-perimeter threshold and a distinct bridge speed") {
        TestConfig config;
        config.print.items.opt("perimeters").set(1);
        config.print.items.opt("perimeter_speed").set(77.0);
        config.print.items.opt("external_perimeter_speed").set(FloatOrPercentage{66.0});
        config.print.items.opt("enable_dynamic_overhang_speeds").set(false);
        config.print.items.opt("bridge_speed").set(33.0);
        config.print.items.opt("small_perimeter_speed").set(FloatOrPercentage{5.0});
        // A huge max_length forces every perimeter (including the bridging
        // one) into the small-perimeter blend range, isolating the bridge
        // exclusion guard: bridging feedrate must stay untouched regardless.
        config.print.items.opt("small_perimeter_min_length").set(0.0);
        config.print.items.opt("small_perimeter_max_length").set(1000000.0);
        config.print.items.opt("overhangs").set(true);
        config.filament.at(0).items.opt("cooling").set(true);
        config.filament.at(0).items.opt("fan_below_layer_time").set(0);
        config.filament.at(0).items.opt("slowdown_below_layer_time").set(0);
        config.filament.at(0).items.opt("bridge_fan_speed").set(100);
        config.print.items.opt("bridge_flow_ratio").set(33.0);

        WHEN("sliced") {
            const std::string gcode = Slic3r::Test::slice({Test::mesh(Test::TestMesh::overhang)}, config);

            const double bridge_speed = config.print.items.opt("bridge_speed").get<double>() * 60.;

            // Ground truth for "is this move bridging": the fan marker, same
            // technique as "Bridging is applied to bridging perimeters" in
            // test_perimeters.cpp, independent of the feedrate we're checking.
            int  fan_speed = 0;
            bool bridging_matches_bridge_speed = true;
            bool bridging_move_seen            = false;
            GCodeReader parser;
            parser.parse_buffer(gcode, [&](GCodeReader &self, const GCodeReader::GCodeLine &line) {
                if (line.cmd_is("M107")) {
                    fan_speed = 0;
                } else if (line.cmd_is("M106")) {
                    line.has_value('S', fan_speed);
                } else if (line.extruding(self) && line.dist_XY(self) > 0 && fan_speed == 255) {
                    bridging_move_seen = true;
                    bridging_matches_bridge_speed =
                        bridging_matches_bridge_speed && is_approx<double>(line.new_F(self), bridge_speed);
                }
            });

            THEN("a bridging move exists in this model") {
                REQUIRE(bridging_move_seen);
            }
            THEN("every bridging move extrudes at the plain bridge speed, unaffected by the blend") {
                REQUIRE(bridging_matches_bridge_speed);
            }
        }
    }
}
