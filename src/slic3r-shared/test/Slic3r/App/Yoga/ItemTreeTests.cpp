///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
///
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <Slic3r/App/Yoga/Item.hpp>

using namespace Slic3r;
using namespace Slic3r::App::Yoga;
using Catch::Matchers::WithinRel;

TEST_CASE("Item 1 child Item::add") {
    Item tree;

    Item* child = new Item;
    tree.append(child);

    REQUIRE(child->parent() == &tree);
    REQUIRE(tree.item_count() == 1);
}

TEST_CASE("Item 1 child set_parent") {
    Item tree;

    Item* child = new Item;
    child->set_parent(&tree);

    REQUIRE(child->parent() == &tree);
    REQUIRE(tree.item_count() == 1);
}

TEST_CASE("Item 1 child constructor parent") {
    Item tree;

    Item* child = new Item(&tree);

    REQUIRE(child->parent() == &tree);
    REQUIRE(tree.item_count() == 1);
}

TEST_CASE("Item 1 child Item::remove") {
    Item tree;

    Item* child = new Item(&tree);

    tree.remove(child);

    REQUIRE(child->parent() == nullptr);
    REQUIRE(tree.item_count() == 0);
}
