#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "imgui.h"
#include "imgui_internal.h"

#include <Slic3r/App/Yoga/Item.hpp>

using namespace Slic3r;
using namespace Slic3r::App::Yoga;
using Catch::Matchers::WithinRel;

/**
 * @brief The ImGuiFixture class
 * @note Each one of these tests should be reproducible with
 * Yoga Playground https://www.yogalayout.dev/playground
 */
struct ImGuiFixture
{
    ImGuiFixture()
    {
        // Setup ImGui context (run once per TEST_CASE)
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();

        ImGui::StyleColorsDark();

        // Setup Dummy context
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1280, 720); // Set a dummy display size
        io.DeltaTime = 1.0f / 60.0f;        // Set a dummy delta-time

        // Explicitly build font atlas to avoid the assertion failure
        unsigned char* tex_pixels = nullptr;
        int tex_w, tex_h;
        io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);

        // Start frame
        ImGui::NewFrame();
        ImGui::PushID(Catch::getResultCapture().getCurrentTestName().c_str());
    }

    ~ImGuiFixture()
    {
        ImGui::PopID();
        ImGui::Render();
        // ImDrawData* draw_data = ImGui::GetDrawData();

        ImGui::DestroyContext(ctx);
    }

    ImGuiContext* ctx;
};

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

    Item* child = new Item(&layout);
    child->set_max_size({10, 15});
    child->set_flex_grow(1.);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child->width(), WithinRel(10, 0.0001));
    REQUIRE_THAT(child->height(), WithinRel(15, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item flex_grow")
{
    Item layout;

    Item* child_left = new Item(&layout);
    child_left->set_min_size({10, 0});

    Item* child_right = new Item(&layout);
    child_right->set_flex_grow(1);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child_left->width(), WithinRel(10, 0.0001));
    REQUIRE_THAT(child_right->width(), WithinRel(90, 0.0001));
}

TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item flex_grow double even")
{
    Item layout;
    layout.set_orientation(Orientation::Horizontal);

    Item* child_left = new Item(&layout);
    child_left->set_flex_grow(1);

    Item* child_right = new Item(&layout);
    child_right->set_flex_grow(1);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child_left->width(), WithinRel(50, 0.0001));
    REQUIRE_THAT(child_right->width(), WithinRel(50, 0.0001));
}

// TEST_CASE_METHOD(ImGuiFixture, "Yoga::Item flex_grow complex")
// {
//     Item layout;
//     layout.set_orientation(Orientation::Horizontal);

//     Item* child_left = new Item(&layout);
//     child_left->set_flex_grow(.3f);

//     Item* child_center = new Item(&layout);
//     child_center->set_flex_grow(.2f);

//     Item* child_right = new Item(&layout);
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

    Item* child_left = new Item(&layout);
    child_left->set_flex_grow(1);

    Item* child_right = new Item(&layout);
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

    Item* child = new Item(&layout);
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

    Item* child = new Item(&layout);
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

    Item* child = new Item(&layout);
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

    Item* child = new Item(&layout);
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

    Item* child = new Item(&layout);
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

    Item* child = new Item(&layout);
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

    Item* child = new Item(&layout);
    child->set_min_size({10, 10});
    child->set_visible(false);

    Item* child_visible = new Item(&layout);
    child_visible->set_flex_grow(1);

    layout.render({}, {100, 50});

    REQUIRE_THAT(child_visible->width(), WithinRel(100, 0.0001));
    REQUIRE_THAT(child_visible->height(), WithinRel(50, 0.0001));
}
