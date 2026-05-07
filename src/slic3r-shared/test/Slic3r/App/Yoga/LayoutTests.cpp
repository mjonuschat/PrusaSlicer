///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <Slic3r/App/Yoga/Item.hpp>
#include <Slic3r/App/Yoga/RootItem.hpp>

#include "ImGuiFixture.hpp"

using namespace Slic3r;
using namespace Slic3r::App::Yoga;
using Catch::Matchers::WithinRel;


TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] render")
{
    RootItem item;

    item.render({}, {100, 50});

    REQUIRE_THAT(item.width(), WithinRel(100, 0.0001));
    REQUIRE_THAT(item.height(), WithinRel(50, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] max_size")
{
    RootItem layout;

    Item* child = layout.emplace_back<Item>();
    child->set_max_size({10, 15});
    child->set_flex_grow(1.);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child->width(), WithinRel(10, 0.0001));
    REQUIRE_THAT(child->height(), WithinRel(15, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] flex_grow")
{
    RootItem layout;

    Item* child_left = layout.emplace_back<Item>();
    child_left->set_min_size({10, 0});

    Item* child_right = layout.emplace_back<Item>();
    child_right->set_flex_grow(1);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child_left->width(), WithinRel(10, 0.0001));
    REQUIRE_THAT(child_right->width(), WithinRel(90, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] flex_grow double even")
{
    RootItem layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child_left = layout.emplace_back<Item>();
    child_left->set_flex_grow(1);

    Item* child_right = layout.emplace_back<Item>();
    child_right->set_flex_grow(1);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child_left->width(), WithinRel(50, 0.0001));
    REQUIRE_THAT(child_right->width(), WithinRel(50, 0.0001));
}

// TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] flex_grow complex")
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

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] gap")
{
    RootItem layout;
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

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] padding")
{
    RootItem layout;
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

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] padding complex")
{
    RootItem layout;
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

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] margin")
{
    RootItem layout;
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

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] margin complex")
{
    RootItem layout;
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

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] aspect ratio")
{
    RootItem layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child = layout.emplace_back<Item>();
    child->set_aspect_ratio(1);
    child->set_max_size({50, 100});

    layout.render({}, {100, 50});

    REQUIRE_THAT(child->width(), WithinRel(50, 0.0001));
    REQUIRE_THAT(child->height(), WithinRel(50, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] center child")
{
    RootItem layout;
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

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] vertical orientation")
{
    RootItem layout;
    layout.set_orientation(Orientation::Vertical);

    Item* top = layout.emplace_back<Item>();
    top->set_flex_grow(1);

    Item* bottom = layout.emplace_back<Item>();
    bottom->set_flex_grow(1);

    layout.render({}, {100, 50});

    REQUIRE_THAT(top->height(), WithinRel(25, 0.0001));
    REQUIRE_THAT(bottom->height(), WithinRel(25, 0.0001));
    REQUIRE_THAT(top->width(), WithinRel(100, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] flex shrink")
{
    RootItem layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child_fixed = layout.emplace_back<Item>();
    child_fixed->set_flex_shrink(0);
    child_fixed->set_width(80);

    Item* child_shrink = layout.emplace_back<Item>();
    child_shrink->set_width(80); // flex_shrink defaults to 1

    layout.render({}, {100, 50});

    // Total = 160, overflow = 60; child_fixed doesn't shrink, child_shrink absorbs all
    REQUIRE_THAT(child_fixed->width(), WithinRel(80, 0.0001));
    REQUIRE_THAT(child_shrink->width(), WithinRel(20, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] absolute positioning")
{
    RootItem layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* flex_child = layout.emplace_back<Item>();
    flex_child->set_flex_grow(1);

    Item* abs_child = layout.emplace_back<Item>();
    abs_child->set_position_type(YGPositionTypeAbsolute);
    abs_child->set_left(20);
    abs_child->set_top(10);
    abs_child->set_min_size({30, 20});

    layout.render({}, {100, 50});

    // Absolute child is out of flow: flex child fills the whole container
    REQUIRE_THAT(flex_child->width(), WithinRel(100, 0.0001));
    // Absolute child sits at its explicit position
    REQUIRE_THAT(abs_child->x(), WithinRel(20, 0.0001));
    REQUIRE_THAT(abs_child->y(), WithinRel(10, 0.0001));
    REQUIRE_THAT(abs_child->width(), WithinRel(30, 0.0001));
    REQUIRE_THAT(abs_child->height(), WithinRel(20, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] flex wrap")
{
    RootItem layout;
    layout.set_orientation(Orientation::Horizontal);
    layout.set_flex_wrap(YGWrapWrap);

    Item* child1 = layout.emplace_back<Item>();
    child1->set_min_size({60, 40});

    Item* child2 = layout.emplace_back<Item>();
    child2->set_min_size({60, 40});

    layout.render({}, {100, 100});

    // child2 doesn't fit on row 1 (60+60=120 > 100), wraps to a new row
    REQUIRE_THAT(child1->height(), WithinRel(40, 0.0001));
    REQUIRE_THAT(child2->y(), WithinRel(40, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] align content center")
{
    RootItem layout;
    layout.set_orientation(Orientation::Horizontal);
    layout.set_flex_wrap(YGWrapWrap);
    layout.set_align_content(YGAlignCenter);

    Item* child1 = layout.emplace_back<Item>();
    child1->set_min_size({60, 40});

    Item* child2 = layout.emplace_back<Item>();
    child2->set_min_size({60, 40});

    layout.render({}, {100, 100});

    // Two rows of 40px (80px total) centered in 100px → 10px top offset
    REQUIRE_THAT(child1->y(), WithinRel(10, 0.0001));
    REQUIRE_THAT(child2->y(), WithinRel(50, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] get_global_pos")
{
    RootItem layout;
    layout.set_orientation(Orientation::Horizontal);
    layout.set_padding(Paddings(5, 10, 0, 0)); // left=5, top=10

    Item* child = layout.emplace_back<Item>();
    child->set_flex_grow(1);

    Item* grandchild = child->emplace_back<Item>();
    grandchild->set_min_size({20, 20});

    layout.render({}, {100, 100});

    // child sits at (5, 10) due to padding; grandchild at (0, 0) within child
    // get_global_pos accumulates positions up the parent_item chain
    Vec2f gpos = grandchild->get_global_pos();
    REQUIRE_THAT(gpos.x(), WithinRel(5, 0.0001));
    REQUIRE_THAT(gpos.y(), WithinRel(10, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "[Yoga::Item] invisible child")
{
    RootItem layout;
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
