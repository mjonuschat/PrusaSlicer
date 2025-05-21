#pragma once
#include <thread>
#include <mutex>
#include <functional>

#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Render/Geometry.hpp"
#include "Slic3r/App/Scene/IGizmo.hpp" // IToolGizmo
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Domain/ObjectID.hpp"

namespace Slic3r::App::Plater {

class SimplifyGizmo : public Scene::IToolGizmo
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
    void render_imgui() override;
    /**@}*/

    /**
     * @name Implementation of IToolGizmo interface
     * @{
     */
    void on_activated() override;
    void on_deactivated() override;
    Scene::ToolType type() const override { return Scene::ToolType::Simplify; }
    /**@}*/

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
        const ModelObject* mo = nullptr;

        Data result;
        std::set<Domain::ObjectID> volume_ids; // is same as result keys - store separate for faster check
    };

    void draw_tool();
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

    bool m_activated = false;

    std::thread m_worker;
    std::mutex m_state_mutex; // guards m_state
    State m_state;            // accessed by both threads

    Configuration m_configuration;
    std::set<Domain::ObjectID> m_volume_ids; // current processing volumes

    // ImGui variable
    bool m_show_wireframe = false;
    size_t m_original_triangle_count = 0;
    size_t m_triangle_count = 0;
    std::string m_mesh_name; // name of the mesh we are working on

    struct Phantom{
        Domain::ObjectID volume_id;
        std::unique_ptr<Render::Geometry> geometry;
    };
    using Phantoms = std::vector<Phantom>;
    Phantoms m_phantoms; // keep phantom geometries
    Render::Material m_material;

    std::vector<Scene::Node*> m_to_enable;
};

} // namespace Slic3r::App::Plater
