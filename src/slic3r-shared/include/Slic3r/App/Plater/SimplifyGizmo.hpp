#pragma once
#include <thread>
#include <mutex>
#include <functional>

#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/Geometry.hpp"
#include "Slic3r/App/Render/Material.hpp"
#include "Slic3r/App/Scene/IGizmo.hpp" // IToolGizmo
#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp" // ISceneSelectionChangedListener
#include "Slic3r/Domain/ObjectID.hpp"

namespace Slic3r::App::Yoga {
class Dialog;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Plater {
class SimplifyDialog;

// Continue development for GLGizmoSimplify permanent link: 
// https://github.com/prusa3d/PrusaSlicer/blob/6fd9846df131c671ac9f944c836536f04d354a53/src/slic3r/GUI/Gizmos/GLGizmoSimplify.hpp
// https://github.com/prusa3d/PrusaSlicer/blob/6fd9846df131c671ac9f944c836536f04d354a53/src/slic3r/GUI/Gizmos/GLGizmoSimplify.cpp
class SimplifyGizmo : public Scene::IToolGizmo, public Biz::Scene::ISceneSelectionChangedListener
{
public:
    using CloseFn = std::function<void()>;
    SimplifyGizmo(
        Render::Device& device,
        PlaterScenePresenter& scene_presenter,
        Biz::ProjectInteractor& project_interactor,
        CloseFn close_fn
    );
    ~SimplifyGizmo() override;
    /**
     * @name Implementation of IGizmo interface
     * @{
     */
    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;
    void on_cycle_prepare() override;
    /**@}*/

    /**
     * @name Implementation of IToolGizmo interface
     * @{
     */
    void on_activated() override;
    void on_deactivated() override;
    Scene::ToolType type() const override { return Scene::ToolType::Simplify; }
    /**@}*/

    /**
     * @name Implementation of ISceneSelectionChangedListener interface
     */
    void on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection) override;

    Yoga::GizmoDialog* ui_dialog() override;
private:
    struct Configuration
    {
        bool use_count = false;
        float decimate_ratio = 50.f; // in percent
        uint32_t wanted_count = 0;   // initialize by percents
        float max_error = 1.;        // maximal quadric error

        void fix_count_by_ratio(size_t triangle_count);
        bool operator==(const Configuration& rhs);
        bool operator!=(const Configuration& rhs);
    };

    struct State {
        //using Data = std::vector<std::unique_ptr<indexed_triangle_set> >;
        using Data = std::map<Domain::ObjectID, std::unique_ptr<indexed_triangle_set> >;
        enum Status {
            idle,
            running,
            cancelling
        };

        Status status = idle;
        int progress = 0; // percent of done work
        Configuration config; // Configuration we started with.
        const Domain::ModelObject* mo = nullptr;

        Data result;
        std::set<Domain::ObjectID> volume_ids; // is same as result keys - store separate for faster check
    };

    void on_selection_change(const Domain::Project& project, const Biz::Scene::ObjectSelection& selection);

    void update_configuration_on_count_change();
    void update_buttons_on_state_changed(bool enable_apply, bool enable_close);
    void deactivate();
    void close();

    void apply_simplify();
    void process();
    bool stop_worker_thread_request();
    void worker_finished();

    void create_mesh_name();
    void init_material();
    void init_model(const std::set<Domain::ObjectID>&); // initialize gl Models from selection
    void update_model(const State::Data& data);
    struct NodeInput{
        Domain::ObjectID volume_id = 0;
        const indexed_triangle_set* its = nullptr; };
    using NodeInputs = std::vector<NodeInput>;
    void set_nodes(const NodeInputs& node_inputs);

    Render::Device& m_device;
    PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor& m_project_interactor;
    CloseFn m_close_fn; // call GizmoManager to close current gizmo

    std::thread m_worker;
    std::mutex m_state_mutex; // guards m_state
    State m_state;            // accessed by both threads

    Configuration m_configuration;
    std::set<Domain::ObjectID> m_volume_ids; // current processing volumes

    // ImGui variable
    bool m_show_wireframe = false;
    size_t m_original_triangle_count = 0;
    size_t m_triangle_count = 0;
    int m_reduction                  = 2;
    std::string m_mesh_name; // name of the mesh we are working on

    struct Phantom{
        Domain::ObjectID volume_id;
        std::unique_ptr<Render::Geometry> geometry;
    };
    using Phantoms = std::vector<Phantom>;
    Phantoms m_phantoms; // keep phantom geometries
    Render::Material m_material;

    std::vector<Scene::Node*> m_to_enable;

    std::unique_ptr<SimplifyDialog> m_dialog;
};

} // namespace Slic3r::App::Plater
