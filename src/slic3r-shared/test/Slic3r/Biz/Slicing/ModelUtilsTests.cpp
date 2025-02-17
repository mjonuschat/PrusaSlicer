#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <Slic3r/Biz/Slicing/ModelUtils.hpp>
#include "Slic3r/Biz/Slicing/TestUtils.hpp"

using namespace Catch;
using Catch::Matchers::Equals;

using Slic3r::Tests::generate_cubes;
using Slic3r::Domain::ModelInstanceList;
using Slic3r::Biz::Slicing::with_limited_instances;
using Slic3r::ModelInstance;


TEST_CASE("With limited instances temporarily removes instances and objects", "[slicing][slicing-model-utils]")
{
    Slic3r::Model model{generate_cubes(5, 5)};

    Slic3r::TriangleMesh cube_mesh = Slic3r::make_cube(10, 10, 10);
    Slic3r::ModelObject* model_object = model.objects.front();
    model_object->add_volume(cube_mesh);
    model_object->add_instance();
    model_object->ensure_on_bed();

    ModelInstance* first_object_second_instance{model.objects.front()->instances[1]};
    ModelInstance* third_object_instance{model.objects[2]->instances.front()};
    ModelInstance* fifth_object_instance{model.objects[4]->instances.front()};

    const ModelInstanceList instances_to_keep{
        model.objects.front()->instances[1],
        model.objects[2]->instances.front(),
        model.objects[4]->instances.front()
    };

    with_limited_instances(model, instances_to_keep, [&](){
        REQUIRE(model.objects.size() == 3);
        CHECK_THAT(model.objects[0]->instances, Equals(ModelInstanceList{first_object_second_instance}));
        CHECK_THAT(model.objects[1]->instances, Equals(ModelInstanceList{third_object_instance}));
        CHECK_THAT(model.objects[2]->instances, Equals(ModelInstanceList{fifth_object_instance}));
    });
}
