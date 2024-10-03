#pragma once
#include <cstddef>
#include <memory>
#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Render/Geometry.hpp"
#include "Slic3r/App/Render/Shader.hpp"
#include "Slic3r/App/Render/TextureManager.hpp"
#include "Slic3r/App/Scene/Scene.hpp"

namespace Slic3r::App {

class TestRenderModule : public Platform::AbstractRenderModule
{
public:
    TestRenderModule();

    void render_scene() override;
    void render_imgui() override;
    void on_scene_mouse_event(const Platform::MouseEvent &e) override;
    void on_scene_keyboard_event(const Platform::KeyboardEvent &e) override;

protected:
    void on_init(Render::Device& device) override;
    void on_screen_resized() override;

private:
    std::unique_ptr<Render::Geometry> m_geometry;
    std::unique_ptr<Render::Geometry> m_geometry2;
    Render::Shader* m_shader{nullptr};
    Render::Shader* m_shader2{nullptr};
    Render::Texture* m_tex{nullptr};

    static constexpr size_t BUF_SIZE = 256;
    char m_text_buffer[BUF_SIZE];
    bool m_main_window_opened{true};
    float m_geom2_scale{1};

    std::unique_ptr<Scene::Scene> m_scene;
};

} // namespace Slic3r::App::SDLTest
