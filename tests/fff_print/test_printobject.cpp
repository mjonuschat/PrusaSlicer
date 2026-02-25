#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "libslic3r/libslic3r.h"
#include "libslic3r/Print.hpp"
#include "libslic3r/Layer.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;
using namespace Catch;
using Domain::FloatOrPercentage;

SCENARIO("PrintObject: object layer heights", "[PrintObject]") {
    GIVEN("20mm cube and default initial config, initial layer height of 2mm") {
        WHEN("generate_object_layers() is called for 2mm layer heights and nozzle diameter of 3mm") {
            Slic3r::Print print;

            TestConfig config;
            config.print.items.opt("first_layer_height").set(FloatOrPercentage{2.0});
            config.print.items.opt("layer_height").set(2.0);
            config.print.items.opt("nozzle_diameter").set(3.0);

            Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);
            SpanOfConstPtrs<Layer> layers = print.objects().front()->layers();
            THEN("The output vector has 10 entries") {
                REQUIRE(layers.size() == 10);
            }
            AND_THEN("Each layer is approximately 2mm above the previous Z") {
                double last = 0.0;
                for (size_t i = 0; i < layers.size(); ++ i) {
                    REQUIRE((layers[i]->print_z - last) == Approx(2.0));
                    last = layers[i]->print_z;
                }
            }
        }
        WHEN("generate_object_layers() is called for 10mm layer heights and nozzle diameter of 11mm") {
            Slic3r::Print print;

            TestConfig config;
            config.print.items.opt("first_layer_height").set(FloatOrPercentage{2.0});
            config.print.items.opt("layer_height").set(10.0);
            config.print.items.opt("nozzle_diameter").set(11.0);
            Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

            SpanOfConstPtrs<Layer> layers = print.objects().front()->layers();
			THEN("The output vector has 3 entries") {
                REQUIRE(layers.size() == 3);
            }
            AND_THEN("Layer 0 is at 2mm") {
                REQUIRE(layers.front()->print_z == Approx(2.0));
            }
            AND_THEN("Layer 1 is at 12mm") {
                REQUIRE(layers[1]->print_z == Approx(12.0));
            }
        }
        WHEN("generate_object_layers() is called for 15mm layer heights and nozzle diameter of 16mm") {
            Slic3r::Print print;

            TestConfig config;
            config.print.items.opt("first_layer_height").set(FloatOrPercentage{2.0});
            config.print.items.opt("layer_height").set(15.0);
            config.print.items.opt("nozzle_diameter").set(16.0);

            Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);
            SpanOfConstPtrs<Layer> layers = print.objects().front()->layers();

			THEN("The output vector has 2 entries") {
                REQUIRE(layers.size() == 2);
            }
            AND_THEN("Layer 0 is at 2mm") {
                REQUIRE(layers[0]->print_z == Approx(2.0));
            }
            AND_THEN("Layer 1 is at 17mm") {
                REQUIRE(layers[1]->print_z == Approx(17.0));
            }
        }
#if 0
        WHEN("generate_object_layers() is called for 15mm layer heights and nozzle diameter of 5mm") {
            Slic3r::Print print;
            Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, {
        		{ "first_layer_height", 2 },
				{ "layer_height", 		15 },
	            { "nozzle_diameter", 	5 }
	        });
			const std::vector<Slic3r::Layer*> &layers = print.objects().front()->layers();
			THEN("The layer height is limited to 5mm.") {
                CHECK(layers.size() == 5);
                double last = 2.0;
                for (size_t i = 1; i < layers.size(); i++) {
                    REQUIRE((layers[i]->print_z - last) == Approx(5.0));
                    last = layers[i]->print_z;
                }
            }
        }
#endif
    }
}

// SCENARIO("PrintObject: extreme mesh coordinates (regression test for coord_t overflow)", "[PrintObject]") {
//     using Slic3r::Biz::Algorithms::TriangleMesh::make_cube;

//     GIVEN("A 20mm cube translated to Y=3000mm (beyond coord_t range when scaled)") {
//         Domain::TriangleMesh cube = make_cube(20.0, 20.0, 20.0);
//         // Center will be at Y=3010mm, scaled = 3,010,000,000 > int32_t max (2,147,483,647)
//         cube.translate(Vec3f(0.0f, 3000.0f, 0.0f));

//         Print print;
//         TestConfig config;
//         config.print.items.opt("layer_height").set(0.2);

//         // This should NOT throw NoLayers error.
//         REQUIRE_NOTHROW(Slic3r::Test::init_and_process_print({cube}, print, config));

//         THEN("Print has layers (slicing succeeded)") {
//             REQUIRE(!print.objects().empty());
//             const PrintObject* obj = print.objects().front();
//             REQUIRE(obj->layers().size() > 0);
//         }

//         AND_THEN("Layers have non-empty slices") {
//             const PrintObject* obj = print.objects().front();
//             size_t non_empty_layers = 0;
//             for (const Layer* layer : obj->layers()) {
//                 if (!layer->lslices.empty()) {
//                     non_empty_layers++;
//                 }
//             }

//             REQUIRE(non_empty_layers == obj->layers().size());
//         }

//         AND_THEN("Instance shift is correct (no overflow)") {
//             const PrintObject* obj = print.objects().front();
//             const PrintInstance& inst = obj->instances().front();
//             REQUIRE(inst.shift().y() > 0); // Should be positive, not negative from overflow.
//         }
//     }
// }
