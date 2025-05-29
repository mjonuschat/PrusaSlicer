///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <Slic3r/App/Yoga/Item.hpp>

#include "ImGuiFixture.hpp"

using namespace Slic3r;
using namespace Slic3r::App::Yoga;
using Catch::Matchers::WithinRel;


TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item render")
{
    Item item;

    item.render({}, {100, 50});

    REQUIRE_THAT(item.width(), WithinRel(100, 0.0001));
    REQUIRE_THAT(item.height(), WithinRel(50, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item max_size")
{
    Item layout;

    Item* child = layout.emplace_back<Item>();
    child->set_max_size({10, 15});
    child->set_flex_grow(1.);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child->width(), WithinRel(10, 0.0001));
    REQUIRE_THAT(child->height(), WithinRel(15, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item flex_grow")
{
    Item layout;

    Item* child_left = layout.emplace_back<Item>();
    child_left->set_min_size({10, 0});

    Item* child_right = layout.emplace_back<Item>();
    child_right->set_flex_grow(1);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child_left->width(), WithinRel(10, 0.0001));
    REQUIRE_THAT(child_right->width(), WithinRel(90, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item flex_grow double even")
{
    Item layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child_left = layout.emplace_back<Item>();
    child_left->set_flex_grow(1);

    Item* child_right = layout.emplace_back<Item>();
    child_right->set_flex_grow(1);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child_left->width(), WithinRel(50, 0.0001));
    REQUIRE_THAT(child_right->width(), WithinRel(50, 0.0001));
}

// TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item flex_grow complex")
// {
//     Item layout;
//     layout.set_orientation(Orientation::Horizontal);

//     Item* child_left = layout.emplace_back<Item>();
//     child_left->set_flex_grow(.3f);

//     Item* child_center = layout.emplace_back<Item>();
//     child_center->set_flex_grow(.2f);

//     Item* child_right = layout.emplace_back<Item>();
//     child_right->set_flex_grow(.5f);

//     layout.render({}, {100, 50});

//     REQUIRE_THAT(child_left->width(), WithinRel(30, 0.0001));
//     REQUIRE_THAT(child_center->width(), WithinRel(20, 0.0001));
//     REQUIRE_THAT(child_right->width(), WithinRel(50, 0.0001));
// }

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item gap")
{
    Item layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child_left = layout.emplace_back<Item>();
    child_left->set_flex_grow(1);

    Item* child_right = layout.emplace_back<Item>();
    child_right->set_flex_grow(1);

    layout.set_gap(10);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child_left->width(), WithinRel(45, 0.0001));
    REQUIRE_THAT(child_right->width(), WithinRel(45, 0.0001));
    REQUIRE_THAT(child_right->x(), WithinRel(55, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item padding")
{
    Item layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child = layout.emplace_back<Item>();
    child->set_flex_grow(1);

    layout.set_padding(10);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child->width(), WithinRel(80, 0.0001));
    REQUIRE_THAT(child->height(), WithinRel(30, 0.0001));
    REQUIRE_THAT(child->x(), WithinRel(10, 0.0001));
    REQUIRE_THAT(child->y(), WithinRel(10, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item padding complex")
{
    Item layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child = layout.emplace_back<Item>();
    child->set_flex_grow(1);

    layout.set_padding(Paddings(5, 10, 15, 20));

    layout.render({}, {100, 50});

    REQUIRE_THAT(child->width(), WithinRel(80, 0.0001));
    REQUIRE_THAT(child->height(), WithinRel(20, 0.0001));
    REQUIRE_THAT(child->x(), WithinRel(5, 0.0001));
    REQUIRE_THAT(child->y(), WithinRel(10, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item margin")
{
    Item layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child = layout.emplace_back<Item>();
    child->set_flex_grow(1);
    child->set_margin(10);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child->width(), WithinRel(80, 0.0001));
    REQUIRE_THAT(child->height(), WithinRel(30, 0.0001));
    REQUIRE_THAT(child->x(), WithinRel(10, 0.0001));
    REQUIRE_THAT(child->y(), WithinRel(10, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item margin complex")
{
    Item layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child = layout.emplace_back<Item>();
    child->set_flex_grow(1);
    child->set_margin(Margins(5, 10, 15, 20));

    layout.render({}, {100, 50});

    REQUIRE_THAT(child->width(), WithinRel(80, 0.0001));
    REQUIRE_THAT(child->height(), WithinRel(20, 0.0001));
    REQUIRE_THAT(child->x(), WithinRel(5, 0.0001));
    REQUIRE_THAT(child->y(), WithinRel(10, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item aspect ratio")
{
    Item layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child = layout.emplace_back<Item>();
    child->set_aspect_ratio(1);
    child->set_max_size({50, 100});

    layout.render({}, {100, 50});

    REQUIRE_THAT(child->width(), WithinRel(50, 0.0001));
    REQUIRE_THAT(child->height(), WithinRel(50, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item center child")
{
    Item layout;
    layout.set_orientation(Orientation::Horizontal);
    layout.set_justify_content(YGJustify::YGJustifyCenter);
    layout.set_align_items(YGAlign::YGAlignCenter);

    Item* child = layout.emplace_back<Item>();
    child->set_min_size({10, 10});

    layout.render({}, {100, 50});

    REQUIRE_THAT(child->width(), WithinRel(10, 0.0001));
    REQUIRE_THAT(child->height(), WithinRel(10, 0.0001));
    REQUIRE_THAT(child->x(), WithinRel(45, 0.0001));
    REQUIRE_THAT(child->y(), WithinRel(20, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item invisible child")
{
    Item layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child = layout.emplace_back<Item>();
    child->set_min_size({10, 10});
    child->set_visible(false);

    Item* child_visible = layout.emplace_back<Item>();
    child_visible->set_flex_grow(1);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child_visible->width(), WithinRel(100, 0.0001));
    REQUIRE_THAT(child_visible->height(), WithinRel(50, 0.0001));
}
