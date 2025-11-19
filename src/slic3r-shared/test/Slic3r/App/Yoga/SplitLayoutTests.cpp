///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <Slic3r/App/Yoga/Item.hpp>
#include <Slic3r/App/Yoga/RootItem.hpp>
#include <Slic3r/App/Yoga/SplitLayout.hpp>

#include "ImGuiFixture.hpp"

using namespace Slic3r;
using namespace Slic3r::App::Yoga;
using Catch::Matchers::WithinRel;


TEST_CASE_METHOD(ImGuiFixture, "[Yoga::SplitLayout] one item")
{
    RootItem item;

    SplitLayout* split = item.emplace_back<SplitLayout>();

    Item* left = split->emplace_back<Item>();
    left->set_min_size({30, YGUndefined});

    item.render({}, {100, 50});

    REQUIRE_THAT(left->width(), WithinRel(30, 0.0001));
    REQUIRE(split->item_count() == 1);
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::SplitLayout] two items")
{
    RootItem item;

    SplitLayout* split = item.emplace_back<SplitLayout>();

    Item* left = split->emplace_back<Item>();
    left->set_min_size({30, YGUndefined});

    Item* right = split->emplace_back<Item>();
    split->set_flex_child(right, true);

    item.render({}, {100, 50});
    item.render({}, {100, 50});

    REQUIRE(split->item_count() == 3);
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::SplitLayout] dynamic")
{
    RootItem item;

    SplitLayout* split = item.emplace_back<SplitLayout>();

    Item* left = split->emplace_back<Item>();

    REQUIRE(split->item_count() == 1);

    Item* right = split->emplace_back<Item>();

    REQUIRE(split->item_count() == 3);

    split->remove(left);

    REQUIRE(split->item_count() == 1);

    split->remove(right);

    REQUIRE(split->item_count() == 0);
}
