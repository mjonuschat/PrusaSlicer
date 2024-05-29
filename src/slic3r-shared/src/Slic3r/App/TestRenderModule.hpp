#pragma once
#include <cstddef>
#include <memory>
#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Render/Buffer.hpp"
#include "Slic3r/App/Render/Shader.hpp"

namespace Slic3r::App {

class TestRenderModule : public Platform::AbstractRenderModule
{
public:
    TestRenderModule();

    void render_scene() override;
    void render_imgui() override;

    void on_scene_mouse_event(const Platform::MouseEvent &e) override;
    void on_scene_keyboard_event(const Platform::KeyboardEvent &e) override;

private:
    Render::Geometry<Render::VertexP3> m_geometry;
    Render::Shader* m_shader{nullptr};

    static constexpr size_t BUF_SIZE = 256;
    char m_text_buffer[BUF_SIZE];
    bool m_main_window_opened{true};
};

} // namespace Slic3r::App::SDLTest
