///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <catch2/catch_test_macros.hpp>

#include <imgui_internal.h>

#include <Slic3r/Biz/Platform/IRenderRequestHandler.hpp>
#include <Slic3r/Biz/Platform/PlatformServices.hpp>

#include <Slic3r/App/Yoga/Item.hpp>
#include <Slic3r/App/Theme.hpp>

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

        m_theme = std::make_unique<Slic3r::App::Theme>();
        Slic3r::App::Yoga::Item::set_theme(m_theme.get());

        // Setup ImGui context (run once per TEST_CASE)
        IMGUI_CHECKVERSION();
        ctx = ImGui::CreateContext();

        ImGui::StyleColorsDark();

        // Setup Dummy context
        ImGuiIO& io    = ImGui::GetIO();
        io.DisplaySize = ImVec2(1'280, 720); // Set a dummy display size
        io.DeltaTime   = 1.0f / 60.0f; // Set a dummy delta-time

        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        io.BackendRendererName = "UnitTest-NoRenderer";

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
    std::unique_ptr<Slic3r::App::Theme> m_theme;

    void request_render() override {}
};
