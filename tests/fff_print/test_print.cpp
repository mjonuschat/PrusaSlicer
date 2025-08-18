#include <catch2/catch_test_macros.hpp>

#include "libslic3r/libslic3r.h"
#include "libslic3r/Print.hpp"
#include "libslic3r/Layer.hpp"
#include "Slic3r/Biz/GCodeReader/GCodeReader.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;
using Biz::GCodeReader::GCodeReader;
using Domain::Percentage;
using Domain::FloatOrPercentage;
using Biz::Print::SerializedConfig;
using Domain::Preset::HwPrinterConfig;
namespace BB = Biz::Algorithms::BoundingBox;

SCENARIO("PrintObject: Perimeter generation", "[PrintObject]") {
    GIVEN("20mm cube and default config") {
        WHEN("make_perimeters() is called")  {
            Slic3r::Print print;
            TestConfig config;
            config.tool.at(0).items.opt("fill_density").set(Percentage{0});
            Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);
			const PrintObject &object = *print.objects().front();
			THEN("67 layers exist in the model") {
                REQUIRE(object.layers().size() == 66);
            }
            THEN("Every layer in region 0 has 1 island of perimeters") {
                for (const Layer *layer : object.layers())
                    REQUIRE(layer->regions().front()->perimeters().size() == 1);
            }
            THEN("Every layer in region 0 has 3 paths in its perimeters list.") {
                for (const Layer *layer : object.layers())
                    REQUIRE(layer->regions().front()->perimeters().items_count() == 3);
            }
        }
    }
}

SCENARIO("Print: Skirt generation", "[Print]") {
    GIVEN("20mm cube and default config") {
        WHEN("Skirts is set to 2 loops")  {
            Slic3r::Print print;
            TestConfig config;
            config.print.items.opt("skirt_height").set(1 );
            config.print.items.opt("skirt_distance").set(1.0);
            config.print.items.opt("skirts").set(2 );
            Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);
            THEN("Skirt Extrusion collection has 2 loops in it") {
                REQUIRE(print.skirt().items_count() == 2);
                REQUIRE(print.skirt().flatten().entities.size() == 2);
            }
        }
    }
}

namespace {
void update(Print& print, Domain::Model& model, const TestConfig& config)
{
    Domain::Bed model_bed;
    Domain::BedInstance bed_instance{model_bed};
    for (const Domain::ModelObject* object : model.objects) {
        for (Domain::ModelInstance* instance : object->instances) {
            bed_instance.model_instances.push_back(instance);
        }
    }
    print.update(model, config, bed_instance, SerializedConfig{}, HwPrinterConfig{});
}
} // namespace

SCENARIO("Print: Changing number of solid surfaces does not cause all surfaces to become internal.", "[Print]") {
    GIVEN("sliced 20mm cube and config with top_solid_surfaces = 2 and bottom_solid_surfaces = 1") {
        TestConfig config;
        config.tool.at(0).items.opt("top_solid_layers").set(2);
        config.tool.at(0).items.opt("bottom_solid_layers").set(1);
        config.print.items.opt("layer_height").set(0.25);
        config.print.items.opt("first_layer_height").set(FloatOrPercentage{0.25});
        Slic3r::Print print;
        Domain::Model model;
        Slic3r::Test::init_print({TestMesh::cube_20x20x20}, print, model, config);
        // Precondition: Ensure that the model has 2 solid top layers (39, 38)
        // and one solid bottom layer (0).
		auto test_is_solid_infill = [&print](size_t obj_id, size_t layer_id) {
		    const Layer &layer = *print.objects()[obj_id]->get_layer((int)layer_id);
		    // iterate over all of the regions in the layer
		    for (const LayerRegion *region : layer.regions()) {
		        // for each region, iterate over the fill surfaces
		        for (const Surface &surface : region->fill_surfaces())
		            CHECK(surface.is_solid());
		    }
		};
        print.process();
        test_is_solid_infill(0,  0); // should be solid
        test_is_solid_infill(0, 79); // should be solid
        test_is_solid_infill(0, 78); // should be solid
        WHEN("Model is re-sliced with top_solid_layers == 3") {
			config.tool.at(0).items.opt("top_solid_layers").set(3);
            update(print, model, config);
            print.process();
            THEN("Print object does not have 0 solid bottom layers.") {
                test_is_solid_infill(0, 0);
            }
            AND_THEN("Print object has 3 top solid layers") {
                test_is_solid_infill(0, 79);
                test_is_solid_infill(0, 78);
                test_is_solid_infill(0, 77);
            }
        }
    }
}

SCENARIO("Print: Brim generation", "[Print]") {
    GIVEN("20mm cube and default config, 1mm first layer width") {
        WHEN("Brim is set to 3mm")  {
	        Slic3r::Print print;
            TestConfig config;
            config.tool.at(0).items.opt("first_layer_extrusion_width").set(FloatOrPercentage{1.0});
            config.print.items.opt("brim_width").set(3.0);
	        Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);
            THEN("Brim Extrusion collection has 3 loops in it") {
                REQUIRE(print.brim().items_count() == 3);
            }
        }
        WHEN("Brim is set to 6mm")  {
	        Slic3r::Print print;
            TestConfig config;
            config.tool.at(0).items.opt("first_layer_extrusion_width").set(FloatOrPercentage{1.0});
            config.print.items.opt("brim_width").set(6.0);
	        Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);
            THEN("Brim Extrusion collection has 6 loops in it") {
                REQUIRE(print.brim().items_count() == 6);
            }
        }
        WHEN("Brim is set to 6mm, extrusion width 0.5mm")  {
	        Slic3r::Print print;
            TestConfig config;
            config.tool.at(0).items.opt("first_layer_extrusion_width").set(FloatOrPercentage{0.5});
            config.print.items.opt("brim_width").set(6.0);
	        Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);
			print.process();
            THEN("Brim Extrusion collection has 12 loops in it") {
                REQUIRE(print.brim().items_count() == 14);
            }
        }
    }
}

TEST_CASE("Ported from Perl", "[Print]") {
    GIVEN("20mm cube") {
        WHEN("Print center is set to 100x100 (test framework default)")  {
            std::string gcode = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, TestConfig{});
            GCodeReader parser;
            Points      extrusion_points;
            parser.parse_buffer(gcode, [&extrusion_points](GCodeReader &self, const GCodeReader::GCodeLine &line)
            {
                if (line.cmd_is("G1") && line.extruding(self) && line.dist_XY(self) > 0)
                    extrusion_points.emplace_back(line.new_XY_scaled(self));
            });
            Vec2d center = unscaled<double>(BB::center(BB::construct(extrusion_points)));
            THEN("print is centered around print_center") {
                REQUIRE(is_approx(center.x(), 100.));
                REQUIRE(is_approx(center.y(), 100.));
            }
        }
    }
    GIVEN("Model with multiple objects") {
        TestConfig config{4};
        for (auto& tool_settings : config.tool) {
            tool_settings.items.opt("nozzle_diameter").set(0.4);
        }
        Print print;
        Domain::Model model;
        Slic3r::Test::init_print({ TestMesh::cube_20x20x20 }, print, model, config);

        // User sets a per-region option, also testing a deep copy of Model.
        Domain::Model model2(model);
        model2.objects.front()->object_settings.overrides.set("fill_density", Percentage{100});
        WHEN("fill_density overridden") {
            update(print, model2, config);
            THEN("region config inherits model object config") {
                REQUIRE(print.get_print_region(0).config().get<Percentage>("fill_density").value == Percentage{100}.value);
            }
        }

        model2.objects.front()->object_settings.overrides.disable("fill_density");
        WHEN("fill_density resetted") {
            update(print, model2, config);
            THEN("region config is resetted") {
                REQUIRE(print.get_print_region(0).config().get<Percentage>("fill_density") == Percentage{20});
            }
        }

        WHEN("extruder is assigned") {
            model2.objects.front()->object_settings.items.opt("extruder").set(3);
            model2.objects.front()->object_settings.overrides.set("perimeter_extruder", 2);
            update(print, model2, config);
            THEN("extruder setting is correctly expanded") {
                REQUIRE(print.get_print_region(0).config().get<int>("infill_extruder") == 3);
            }

            THEN("extruder setting *does* override explicitely specified extruders") {
                REQUIRE(print.get_print_region(0).config().get<int>("perimeter_extruder") == 3);
            }
        }
    }
}
