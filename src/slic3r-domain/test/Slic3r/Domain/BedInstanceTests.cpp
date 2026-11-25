#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"

#include <unordered_map>

using namespace Slic3r::Domain;

namespace {

Bed make_bed()
{
    BedCreationData data;
    data.type    = BedType::Rectangle;
    data.contour = {{0., 0.}, {200., 0.}, {200., 200.}, {0., 200.}};
    return Bed::create(data);
}

}

TEST_CASE("BedInstance name defaults to Bed N", "[BedInstance]")
{
    const Bed bed = make_bed();
    BedInstance instance{bed};
    instance.set_index(2);

    CHECK(instance.name() == "Bed 2");
}

TEST_CASE("BedInstance custom name replaces the default", "[BedInstance]")
{
    const Bed bed = make_bed();
    BedInstance instance{bed};
    instance.set_index(2);
    instance.set_custom_name("Hinges");

    CHECK(instance.name() == "Hinges");
    CHECK(instance.custom_name() == "Hinges");
    CHECK(instance.label() == "2");
}

TEST_CASE("BedInstance custom name survives renumbering", "[BedInstance]")
{
    const Bed bed = make_bed();
    BedInstance instance{bed};
    instance.set_index(2);
    instance.set_custom_name("Hinges");
    instance.set_index(5);

    CHECK(instance.name() == "Hinges");
    CHECK(instance.label() == "5");
}

TEST_CASE("BedInstance empty custom name resets to default", "[BedInstance]")
{
    const Bed bed = make_bed();
    BedInstance instance{bed};
    instance.set_index(5);
    instance.set_custom_name("Hinges");
    instance.set_custom_name("");

    CHECK(instance.name() == "Bed 5");
    CHECK(instance.custom_name().empty());
}

TEST_CASE(
    "BedInstance custom name is trimmed and stripped of control characters",
    "[BedInstance]"
)
{
    const Bed bed = make_bed();
    BedInstance instance{bed};
    instance.set_index(1);

    instance.set_custom_name("  A\nB\t ");
    CHECK(instance.custom_name() == "AB");

    instance.set_custom_name("   ");
    CHECK(instance.custom_name().empty());
    CHECK(instance.name() == "Bed 1");
}

TEST_CASE("BedInstance copy keeps the custom name", "[BedInstance]")
{
    const Bed bed = make_bed();
    BedInstance instance{bed};
    instance.set_index(3);
    instance.set_custom_name("Hinges");

    const std::unordered_map<const Bed*, const Bed*> beds{{&bed, &bed}};
    const std::unordered_map<ModelInstance*, ModelInstance*> model_instances;
    const std::unique_ptr<BedInstance> copy = instance.copy(beds, model_instances);

    CHECK(copy->name() == "Hinges");
    CHECK(copy->label() == "3");
}
