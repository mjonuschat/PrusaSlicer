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

    virtual void register_commands() override;

    void init_render();
    void init_scene();

    void render_scene_render();
    void render_scene_scene();

    void render_object_hud(const Scene::Node& n, const Eigen::AlignedBox<float, 2>& screen_bounding_box);

    void remove_highlighted();
    void reset_highlighted(const Scene::Node::NodeList& nodes_to_highlight, const Render::Material& material);

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
    bool m_render_low{false};

    std::unique_ptr<Scene::Scene> m_scene;
    Scene::Node::NodeList m_highlighted_nodes;

    int m_last_mouse_x{0};
    int m_last_mouse_y{0};
};

} // namespace Slic3r::App::SDLTest
