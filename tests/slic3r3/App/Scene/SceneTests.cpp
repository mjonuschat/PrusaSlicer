#include <catch2/catch.hpp>
#include <Slic3r/App/Scene/Scene.hpp>

TEST_CASE("Scene basic transform", "[Node]") {
    using namespace Slic3r::App::Scene;

    std::unique_ptr<Node> root = std::make_unique<Node>();
    root->add_child(new Node);


}
