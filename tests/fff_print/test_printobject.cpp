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

            TestConfig config{1, 3.0};
            config.print.items.opt("first_layer_height").set(FloatOrPercentage{2.0});
            config.print.items.opt("layer_height").set(2.0);

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

            TestConfig config{1, 11.0};
            config.print.items.opt("first_layer_height").set(FloatOrPercentage{2.0});
            config.print.items.opt("layer_height").set(10.0);
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

            TestConfig config{1, 16.0};
            config.print.items.opt("first_layer_height").set(FloatOrPercentage{2.0});
            config.print.items.opt("layer_height").set(15.0);

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
    }
}
