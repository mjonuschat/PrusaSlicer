#include <catch2/catch.hpp>

#include "Slic3r/App/Scene/Scene.hpp"

TEST_CASE("Scene basic transform", "[Node]") {
    using namespace Slic3r;
    using namespace Slic3r::App::Scene;
    using Catch::Matchers::WithinRel;

    Scene scene;

    auto* n1 = new Node;

    REQUIRE(n1->parent() == nullptr);

    scene.add_child(n1);

    REQUIRE(n1->parent() != nullptr);

    auto* n2 = new Node;

    scene.add_child(n2, n1);

    {
        const auto& n1_world = n1->world_transform().block<3, 1>(0, 3);
        REQUIRE_THAT((n1_world.x()), WithinRel(0, 0.0001));
        REQUIRE_THAT((n1_world.y()), WithinRel(0, 0.0001));
        REQUIRE_THAT((n1_world.z()), WithinRel(0, 0.0001));

        const auto& n2_world = n2->world_transform().block<3, 1>(0, 3);
        REQUIRE_THAT((n2_world.x()), WithinRel(0, 0.0001));
        REQUIRE_THAT((n2_world.y()), WithinRel(0, 0.0001));
        REQUIRE_THAT((n2_world.z()), WithinRel(0, 0.0001));
    }

    Transform3f t = Slic3r::Transform3f::Identity();
    t.translate(Vec3f{1, 2, 3});
    n1->set_local_transform(t.matrix());

    {
        const auto& n1_world = n1->world_transform().block<3, 1>(0, 3);
        REQUIRE_THAT((n1_world.x()), WithinRel(1, 0.0001));
        REQUIRE_THAT((n1_world.y()), WithinRel(2, 0.0001));
        REQUIRE_THAT((n1_world.z()), WithinRel(3, 0.0001));

        const auto& n2_world = n2->world_transform().block<3, 1>(0, 3);
        REQUIRE_THAT((n2_world.x()), WithinRel(1, 0.0001));
        REQUIRE_THAT((n2_world.y()), WithinRel(2, 0.0001));
        REQUIRE_THAT((n2_world.z()), WithinRel(3, 0.0001));
    }

    t = Slic3r::Transform3f::Identity();
    t.translate(Vec3f{-1, -2, -3});
    n2->set_local_transform(t.matrix());

    {
        const auto& n1_world = n1->world_transform().block<3, 1>(0, 3);
        REQUIRE_THAT((n1_world.x()), WithinRel(1, 0.0001));
        REQUIRE_THAT((n1_world.y()), WithinRel(2, 0.0001));
        REQUIRE_THAT((n1_world.z()), WithinRel(3, 0.0001));

        const auto& n2_world = n2->world_transform().block<3, 1>(0, 3);
        REQUIRE_THAT((n2_world.x()), WithinRel(0, 0.0001));
        REQUIRE_THAT((n2_world.y()), WithinRel(0, 0.0001));
        REQUIRE_THAT((n2_world.z()), WithinRel(0, 0.0001));
    }
}
