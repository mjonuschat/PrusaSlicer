///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <catch2/catch_test_macros.hpp>

#include <imgui_internal.h>

#include <Slic3r/Biz/Platform/IRenderRequestHandler.hpp>
#include <Slic3r/Biz/Platform/PlatformServices.hpp>

/**
 * @brief The ImGuiFixture class
 * @note Each one of these tests should be reproducible with
 * Yoga Playground https://www.yogalayout.dev/playground
 */
struct ImGuiFixture : public Slic3r::Biz::Platform::IRenderRequestHandler
{
    ImGuiFixture()
    {
        Slic3r::Biz::Platform::PlatformServices::instance().set_render_request_handler(this);

        // Setup ImGui context (run once per TEST_CASE)
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();

        ImGui::StyleColorsDark();

        // Setup Dummy context
        ImGuiIO& io    = ImGui::GetIO();
        io.DisplaySize = ImVec2(1'280, 720); // Set a dummy display size
        io.DeltaTime   = 1.0f / 60.0f; // Set a dummy delta-time

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

    void request_render() override {}
};
