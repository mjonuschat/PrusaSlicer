#pragma once

#include "ScenePresenter.hpp"
#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::App::Plater {

class GizmoDataFactory;
class ScenePresenter;

class TranslationGizmo : public IToolGizmo {
public:
    TranslationGizmo(
        GizmoDataFactory& data_factory,
        ScenePresenter& scene_provider,
        Biz::Scene::SceneInteractor& scene_interactor
    )
        : m_data_factory(data_factory)
        , m_scene_provider(scene_provider)
        , m_scene_interactor(scene_interactor)
    {}

    GizmoActivationState on_mouse(GizmoEventContext& ctx, bool only_active) override;
    void clear_highlight();
    void on_transient_mouse(GizmoEventContext& ctx) override;
    void on_cycle_prepare() override;
    void on_activated() override;
    void on_deactivated() override;
    ToolType type() const override { return ToolType::Translation; }

private:
    GizmoDataFactory& m_data_factory;
    ISceneProvider& m_scene_provider;
    Biz::Scene::SceneInteractor& m_scene_interactor;
    Biz::Scene::TransformMemento m_xform_memento;
    Scene::Ray m_translation_ray;
    double m_start_t{0};
    bool m_activated{false};
    bool m_highlighted{false};
};

}