#include <catch2/catch_test_macros.hpp>

#include "libslic3r/Layer.hpp"
#include "libslic3r/Print.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;
using Domain::Percentage;

SCENARIO("BOSS alternate extra perimeter adds a wall every other layer", "[boss][perimeter]")
{
    GIVEN("a cube with alternate_extra_perimeter enabled, one perimeter and non-zero fill density") {
        Slic3r::Print print;
        TestConfig config;
        config.print.items.opt("perimeters").set(1);
        config.print.items.opt("fill_density").set(Percentage{15});
        config.print.items.opt("spiral_vase").set(false);
        config.print.items.opt("alternate_extra_perimeter").set(true);
        Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);
        const PrintObject& object = *print.objects().front();

        THEN("the odd layer (index 1) has one more perimeter loop than the even layer (index 0)") {
            REQUIRE(object.layers().size() > 1);
            const size_t even_loops = object.layers()[0]->regions().front()->perimeters().items_count();
            const size_t odd_loops  = object.layers()[1]->regions().front()->perimeters().items_count();
            REQUIRE(odd_loops == even_loops + 1);
        }
    }
}
