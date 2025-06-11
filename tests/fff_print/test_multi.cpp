#include <catch2/catch_test_macros.hpp>

#include <numeric>
#include <sstream>

#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/Geometry.hpp"
#include "libslic3r/Geometry/ConvexHull.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/libslic3r.h"
#include "Slic3r/Biz/GCodeReader/GCodeReader.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace std::literals;
using Biz::GCodeReader::GCodeReader;
using Test::TestConfig;
using Domain::VolumeSettings;
using Domain::FloatOrPercentage;
using Domain::Percentage;

SCENARIO("Basic tests", "[Multi]")
{
    WHEN("Slicing multi-material print with non-consecutive extruders") {

        Test::TestConfig config{4};
        for (auto& tool_settings : config.tool) {
            tool_settings.items.opt("nozzle_diameter").set(0.6);
        }

        config.print.items.opt("perimeter_extruder").set(2);
        config.print.items.opt("solid_infill_extruder").set(2);
        config.print.items.opt("infill_extruder").set(4);
        config.print.items.opt("support_material_extruder").set(0);
        std::string gcode = Slic3r::Test::slice({ Slic3r::Test::TestMesh::cube_20x20x20 }, config);
        THEN("Sliced successfully") {
            REQUIRE(! gcode.empty());
        }
        THEN("T3 toolchange command found") {
            bool T1_found = gcode.find("\nT3\n") != gcode.npos;
            REQUIRE(T1_found);
        }
    }
    WHEN("Slicing with multiple skirts with a single, non-zero extruder") {
        Test::TestConfig config{4};
        for (auto& tool_settings : config.tool) {
            tool_settings.items.opt("nozzle_diameter").set(0.6);
        }

        config.print.items.opt("perimeter_extruder").set(2);
        config.print.items.opt("solid_infill_extruder").set(2);
        config.print.items.opt("infill_extruder").set(4);
        config.print.items.opt("support_material_extruder").set(0);
        config.print.items.opt("support_material_interface_extruder").set(2);

        std::string gcode = Slic3r::Test::slice({ Slic3r::Test::TestMesh::cube_20x20x20 }, config);
        THEN("Sliced successfully") {
            REQUIRE(! gcode.empty());
        }
    }
}

SCENARIO("Ooze prevention", "[Multi]")
{
    TestConfig config{4};

    for (auto& tool_settings : config.tool) {
        tool_settings.items.opt("nozzle_diameter").set(0.6);
    }
    config.print.items.opt("raft_layers").set(2);
    config.print.items.opt("infill_extruder").set(2);
    config.print.items.opt("solid_infill_extruder").set(3);
    config.print.items.opt("support_material_extruder").set(4);
    config.print.items.opt("ooze_prevention").set(true);

    config.tool.at(0).items.opt("extruder_offset").set(Vec2d{0, 0});
    config.tool.at(1).items.opt("extruder_offset").set(Vec2d{20, 0});
    config.tool.at(2).items.opt("extruder_offset").set(Vec2d{0, 20});
    config.tool.at(3).items.opt("extruder_offset").set(Vec2d{20, 20});
    config.filament.at(0).items.opt("temperature").set(200);
    config.filament.at(1).items.opt("temperature").set(180);
    config.filament.at(2).items.opt("temperature").set(170);
    config.filament.at(3).items.opt("temperature").set(160);
    config.filament.at(0).items.opt("first_layer_temperature").set(206);
    config.filament.at(1).items.opt("first_layer_temperature").set(186);
    config.filament.at(2).items.opt("first_layer_temperature").set(166);
    config.filament.at(3).items.opt("first_layer_temperature").set(156);
    // test that it doesn't crash when this is supplied
    config.printer.items.opt("toolchange_gcode").set("T[next_extruder] ;toolchange" );

    // Since July 2019, PrusaSlicer only emits automatic Tn command in case that the toolchange_gcode is empty
    // The "T[next_extruder]" is therefore needed in this test.

    std::string gcode = Slic3r::Test::slice({ Slic3r::Test::TestMesh::cube_20x20x20 }, config);

    GCodeReader parser;
    int         tool = -1;
    int         tool_temp[] = { 0, 0, 0, 0};
    Points      toolchange_points;
    Points      extrusion_points;
    parser.parse_buffer(gcode, [&tool, &tool_temp, &toolchange_points, &extrusion_points, &config]
        (GCodeReader &self, const GCodeReader::GCodeLine &line)
    {
        // if the command is a T command, set the the current tool
        if (boost::starts_with(line.cmd(), "T")) {
            // Ignore initial toolchange.
            if (tool != -1) {
                int expected_temp = is_approx<double>(self.z(), config.print.items.opt("first_layer_height").get<FloatOrPercentage>().get_abs_value(1.0) + config.printer.items.opt("z_offset").get<double>()) ?
                    config.filament.at(tool).items.opt("first_layer_temperature").get<int>() :
                    config.filament.at(tool).items.opt("temperature").get<int>();
                if (tool_temp[tool] != expected_temp + config.print.items.opt("standby_temperature_delta").get<int>())
                    throw std::runtime_error("Standby temperature was not set before toolchange.");
                toolchange_points.emplace_back(self.xy_scaled());
            }
            tool = atoi(line.cmd().data() + 1);
        } else if (line.cmd_is("M104") || line.cmd_is("M109")) {
            // May not be defined on this line.
            int t = tool;
            line.has_value('T', t);
            // Should be available on this line.
            int s;
            if (! line.has_value('S', s))
                throw std::runtime_error("M104 or M109 without S");

            // Following is obsolete. The first printing extruder is newly set to its first layer temperature immediately, not to the standby.
            //if (tool_temp[t] == 0 && s != print_config.first_layer_temperature.get_at(t) + print_config.standby_temperature_delta)
            //    throw std::runtime_error("initial temperature is not equal to first layer temperature + standby delta");

            tool_temp[t] = s;
        } else if (line.cmd_is("G1") && line.extruding(self) && line.dist_XY(self) > 0) {
            extrusion_points.emplace_back(line.new_XY_scaled(self) + scaled<coord_t>(config.tool.at(tool).items.opt("extruder_offset").get<Vec2d>()));
        }
    });

    Polygon convex_hull = Geometry::convex_hull(extrusion_points);
    
    // THEN("all nozzles are outside skirt at toolchange") {
    //     Points t;
    //     sort_remove_duplicates(toolchange_points);
    //     size_t inside = 0;
    //     for (const auto &point : toolchange_points)
    //         for (const Vec2d &offset : print_config.extruder_offset.values) {
    //             Point p = point + scaled<coord_t>(offset);
    //             if (convex_hull.contains(p))
    //                 ++ inside;
    //         }
    //     REQUIRE(inside == 0);
    // }

#if 0
    require "Slic3r/SVG.pm";
    Slic3r::SVG::output(
        "ooze_prevention_test.svg",
        no_arrows   => 1,
        polygons    => [$convex_hull],
        red_points  => \@t,
        points      => \@toolchange_points,
    );
#endif
    
    THEN("all toolchanges happen within expected area") {
        // offset the skirt by the maximum displacement between extruders plus a safety extra margin
        const float delta = scaled<float>(20. * sqrt(2.) + 1.);
        Polygon outer_convex_hull = expand(convex_hull, delta).front();
        size_t inside = std::count_if(toolchange_points.begin(), toolchange_points.end(), [&outer_convex_hull](const Point &p){ return Slic3r::Biz::Algorithms::Polygon::contains(outer_convex_hull, p); });
        REQUIRE(inside == toolchange_points.size());
    }
}

std::string slice_stacked_cubes(const TestConfig &config, const VolumeSettings &volume1config, const VolumeSettings &volume2config)
{
    Model        model;
    ModelObject *object = model.add_object();
    object->name = "object.stl";
    ModelVolume *v1 = object->add_volume(Test::mesh(Test::TestMesh::cube_20x20x20));
    v1->set_material_id("lower_material");
    v1->volume_settings = volume1config;
    ModelVolume *v2 = object->add_volume(Test::mesh(Test::TestMesh::cube_20x20x20));
    v2->set_material_id("upper_material");
    v2->translate(0., 0., 20.);
    v2->volume_settings = volume2config;
    object->add_instance();
    object->ensure_on_bed();
    Print print;
    print.auto_assign_extruders(object);
    THEN("auto_assign_extruders() assigned correct extruder to first volume") {
        REQUIRE(v1->config.extruder() == 1);
    }
    THEN("auto_assign_extruders() assigned correct extruder to second volume") {
        REQUIRE(v2->config.extruder() == 2);
    }
    print.apply(model, config, {}, {}, {});
    print.validate();
    return Test::gcode(print);
}

SCENARIO("Stacked cubes", "[Multi]")
{
    VolumeSettings lower_config;

    lower_config.overrides.set("extruder", 1);
    lower_config.overrides.set("bottom_solid_layers", 0);
    lower_config.overrides.set("top_solid_layers", 1);

    VolumeSettings upper_config;

    upper_config.overrides.set("extruder", 2);
    upper_config.overrides.set("bottom_solid_layers", 1);
    upper_config.overrides.set("top_solid_layers", 0);

    static constexpr const double solid_infill_speed = 99;
    TestConfig config{4};

    for (auto& tool_settings : config.tool) {
        tool_settings.items.opt("nozzle_diameter").set(0.6);
    }
    config.print.items.opt("fill_density").set(Percentage{0});
    config.print.items.opt("solid_infill_speed").set(FloatOrPercentage{solid_infill_speed});
    config.print.items.opt("top_solid_infill_speed").set(FloatOrPercentage{solid_infill_speed});

    // for preventing speeds from being altered
    for (auto& filament_settings : config.filament) {
        filament_settings.items.opt("cooling").set(false);
    }

    // for preventing speeds from being altered
    config.print.items.opt("first_layer_speed").set(FloatOrPercentage{Percentage{100}});

    auto test_shells = [](const std::string &gcode) {
        GCodeReader       parser;
        int               tool = -1;
        // Scaled Z heights.
        std::set<coord_t> T0_shells, T1_shells;
        parser.parse_buffer(gcode, [&tool, &T0_shells, &T1_shells]
            (GCodeReader &self, const GCodeReader::GCodeLine &line)
        {
            if (boost::starts_with(line.cmd(), "T")) {
                tool = atoi(line.cmd().data() + 1);
            } else if (line.cmd() == "G1" && line.extruding(self) && line.dist_XY(self) > 0) {
                if (is_approx<double>(line.new_F(self), solid_infill_speed * 60.) && (tool == 0 || tool == 1))
                    (tool == 0 ? T0_shells : T1_shells).insert(scaled<coord_t>(self.z()));
            }
        });
        return std::make_pair(T0_shells, T1_shells);
    };

    WHEN("Interface shells disabled") {
        std::string gcode = slice_stacked_cubes(config, lower_config, upper_config);
        auto [t0, t1] = test_shells(gcode);
        THEN("no interface shells") {
            REQUIRE(t0.empty());
            REQUIRE(t1.empty());
        }
    }
    WHEN("Interface shells enabled") {
        config.print.items.opt("interface_shells").set(true);
        std::string gcode = slice_stacked_cubes(config, lower_config, upper_config);
        auto [t0, t1] = test_shells(gcode);
        THEN("top interface shells") {
            REQUIRE(t0.size() == lower_config.overrides.get("top_solid_layers")->get<int>());
        }
        THEN("bottom interface shells") {
            REQUIRE(t1.size() == upper_config.overrides.get("bottom_solid_layers")->get<int>());
        }
    }
    WHEN("Slicing with auto-assigned extruders") {
        TestConfig config{4};
        for (auto& tool_settings : config.tool) {
            tool_settings.items.opt("nozzle_diameter").set(0.6);
        }
        config.print.items.opt("layer_height").set(0.4);
        config.print.items.opt("first_layer_height").set(FloatOrPercentage{0.4});
        config.print.items.opt("skirts").set(0);
        std::string gcode = slice_stacked_cubes(config, VolumeSettings{}, VolumeSettings{});
        GCodeReader       parser;
        int               tool = -1;
        // Scaled Z heights.
        std::set<coord_t> T0_shells, T1_shells;
        parser.parse_buffer(gcode, [&tool, &T0_shells, &T1_shells](GCodeReader &self, const GCodeReader::GCodeLine &line)
        {
            if (boost::starts_with(line.cmd(), "T")) {
                tool = atoi(line.cmd().data() + 1);
            } else if (line.cmd() == "G1" && line.extruding(self) && line.dist_XY(self) > 0) {
                if (tool == 0 && self.z() > 20)
                    // Layers incorrectly extruded with T0 at the top object.
                    T0_shells.insert(scaled<coord_t>(self.z()));
                else if (tool == 1 && self.z() < 20)
                    // Layers incorrectly extruded with T1 at the bottom object.
                    T1_shells.insert(scaled<coord_t>(self.z()));
            }
        });
        THEN("T0 is never used for upper object") {
            REQUIRE(T0_shells.empty());
        }
        THEN("T0 is never used for lower object") {
            REQUIRE(T1_shells.empty());
        }
    }
}
