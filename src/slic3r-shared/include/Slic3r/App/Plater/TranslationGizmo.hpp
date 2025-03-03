#pragma once

#include "PlaterScenePresenter.hpp"
#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Render/Device.hpp"

namespace Slic3r::App::Scene {
class GeometryDataFactory;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Plater {

class TranslationGizmo : public Scene::IToolGizmo
{
public:
    TranslationGizmo(
        Render::Device& device,
        Scene::GeometryDataFactory& data_factory,
        PlaterScenePresenter& scene_provider,
        Biz::Scene::SceneInteractor& scene_interactor
    )
        : m_device(device)
        , m_data_factory(data_factory)
        , m_scene_provider(scene_provider)
        , m_scene_interactor(scene_interactor)
    {}

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;
    void clear_highlight();
    void on_transient_mouse(Scene::GizmoEventContext& ctx) override;
    void on_cycle_prepare() override;
    void on_activated() override;
    void on_deactivated() override;
    Scene::ToolType type() const override { return Scene::ToolType::Translation; }

private:
    Render::Device& m_device;
    Scene::GeometryDataFactory& m_data_factory;
    Scene::ISceneProvider& m_scene_provider;
    Biz::Scene::SceneInteractor& m_scene_interactor;
    Biz::Scene::TransformMemento m_xform_memento;
    Scene::Ray m_translation_ray;
    double m_start_t{0};
    bool m_dragging{ false };
    bool m_activated{false};
    bool m_highlighted{false};
};

}