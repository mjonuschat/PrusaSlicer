#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"

namespace Slic3r::App::Scene {
class ISceneProvider;
} // namespace Slic3r::App::Scene

namespace Slic3r::Biz::Scene {
class SceneInteractor;
} // namespace Slic3r::Biz::Scene

namespace Slic3r::App::Plater {

enum class ContextMenuType
{
    Scene,
    Bed,
    Object,
    Volume
};

class IShowContextMenuListener
{
public:
    virtual ~IShowContextMenuListener()                                        = default;
    virtual void on_show_context_menu(ContextMenuType type, Domain::Vec2f mouse_position) = 0;
};

class ContextMenuGizmo : public Scene::IGizmo, public WithListeners<IShowContextMenuListener>
{
public:
    ContextMenuGizmo(
        Biz::Scene::SceneInteractor& m_scene_interactor,
        Scene::ISceneProvider& scene_provider
    );

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;

private:
    void invoke_show_context_menu(ContextMenuType type, Domain::Vec2f mouse_position);

private:
    Biz::Scene::SceneInteractor& m_scene_interactor;
    Scene::ISceneProvider& m_scene_provider;
};

} // namespace Slic3r::App::Plater
