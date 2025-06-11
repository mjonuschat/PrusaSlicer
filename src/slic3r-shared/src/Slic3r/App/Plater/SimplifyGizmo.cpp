#include "Slic3r/App/Plater/SimplifyGizmo.hpp"

#include <exception>
#include <chrono>

#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp" // main_thread_dispatcher + render_request_handler
#include "Slic3r/Biz/Algorithms/QuadricEdgeCollapse.hpp"

#include "libslic3r/Model.hpp"
#include "libslic3r/I18N.hpp" // translation

#include "imgui/imgui.h"

// TODO: 
// 1. add Notification: there is volume with a lot of small triangles and suggest simplify
// 2. Esc key down: cancel simplification
// 3. add disable into ImguiWindow
// 4. validate volume exchange(Scne::Node are correct but object list is invalid)
namespace Slic3r::App::Plater {

using Slic3r::App::Render::GeometryBuilder;
using Slic3r::App::Render::Geometry;
using Slic3r::App::Render::Device;
using Slic3r::App::Scene::GizmoActivationState;
using Slic3r::App::Scene::GizmoEventContext;
using Slic3r::App::Scene::Node;
using Slic3r::App::Scene::NodeBuilder;
using Slic3r::Biz::Scene::SceneInteractor;
using Slic3r::Biz::Scene::Selection;
using Slic3r::Biz::ProjectInteractor;
using Slic3r::Biz::Platform::IMainThreadDispatcher;
using Slic3r::Biz::Platform::PlatformServices;
using Slic3r::Domain::ElementRef;
using Slic3r::Domain::Project;
using Slic3r::Domain::ObjectID;

using Slic3r::Biz::Algorithms::its_quadric_edge_collapse;

namespace {
// Static variables
const float wireframe_width = 0.5f;
const ColorRGBA wireframe_color = ColorRGBA::BLUE();

struct SimplifyNodeTag {};
bool is_simplify_node(const Node* n){
    const SimplifyNodeTag* tag = n->tag_of_type<SimplifyNodeTag>();
    return tag != nullptr;
}

// cancel exception
class SimplifyCanceledException : public std::exception {
public: const char* what() const throw() { return _u8L("Model simplification has been canceled").c_str(); }};

// to prevent freezing when move in gui
// delay before process in [ms]
static std::chrono::duration<long int, std::milli> prcess_delay = std::chrono::milliseconds(250);

using SelectedVolumes = std::vector<const ModelVolume*>;
SelectedVolumes get_selected_volumes(const Selection& selection, const Project& project)
{
    SelectedVolumes volumes;
    for (const ElementRef& element : selection.elements) {
        if (element.volume_id == 0) { // is object
            const ModelObject* object = project.find_object_by_id(element.object_id);
            volumes.insert(volumes.end(), object->volumes.begin(), object->volumes.end());
        } else { // volume selected
            volumes.push_back(project.find_volume_by_id(element.object_id, element.volume_id));
        }
    }
    if (volumes.size() <= 1)
        return volumes;

    // instance point on same volume
    std::sort(volumes.begin(), volumes.end());
    volumes.erase(std::unique(volumes.begin(), volumes.end()), volumes.end());

    return volumes;
}

using SelectedVolumeIds = std::set<ObjectID>;
SelectedVolumeIds get_volume_ids(const Selection& selection, const Project& project)
{
    SelectedVolumeIds ids;
    for (const ElementRef& element : selection.elements) {
        if (element.volume_id == 0) { // is object
            const ModelObject* object = project.find_object_by_id(element.object_id);
            for (const ModelVolume* volume : object->volumes)
                ids.emplace(volume->id());
        } else { // volume selected
            ids.emplace(element.volume_id);
        }
    }
    return ids;
}

const ModelVolume* get_volume_by_id(const ObjectID& volume_id, const Project& project) {
    for (ModelObject* object : project.model().objects) {
        for (ModelVolume* volume : object->volumes) {
            if (volume->id() == volume_id) {
                return volume;
            }
        }
    }
    return nullptr;
}

ModelVolume* get_volume_by_id(const ObjectID& volume_id, Project& project) {
    for (ModelObject* object : project.model().objects) {
        for (ModelVolume* volume : object->volumes) {
            if (volume->id() == volume_id) {
                return volume;
            }
        }
    }
    return nullptr;
}
} // namespace

SimplifyGizmo::SimplifyGizmo(
    Device& device,
    PlaterScenePresenter& scene_presenter,
    ProjectInteractor& project_interactor,
    CloseFn close_fn
)
    : m_device(device)
    , m_scene_presenter(scene_presenter)
    , m_project_interactor(project_interactor)
    , m_close_fn(close_fn)
{
    init_material();
}

SimplifyGizmo::~SimplifyGizmo() {
    stop_worker_thread_request();
    if (m_worker.joinable())
        m_worker.join();
}

GizmoActivationState SimplifyGizmo::on_mouse(GizmoEventContext& ctx, bool only_active)
{
    return GizmoActivationState::Inactive;
}

void SimplifyGizmo::on_cycle_prepare()
{ 
}

void SimplifyGizmo::render_imgui() 
{
    const SceneInteractor& scene_interactor = m_project_interactor.scene_interactor();
    const Selection& selection = scene_interactor.selection();
    if (selection.elements.empty())
        close(); // gizmo should not be open with empty selection

    const Project& project = m_project_interactor.selected_project();
    std::set<ObjectID> act_volume_ids = get_volume_ids(selection, project);
    // Check selection of new volume (or change)
    // Do not reselect object when processing
    if (m_volume_ids != act_volume_ids) {
        init_model(act_volume_ids);

        // Start processing. If we switched from another object, process will
        // stop the background thread and it will restart itself later.
        process();
        return;
    }

    //ASSERT(m_activated, "Draw only activated window");
    int flag = ImGuiWindowFlags_AlwaysAutoResize | 
               ImGuiWindowFlags_NoResize |
               ImGuiWindowFlags_NoCollapse;
    
    ImGui::Begin(_u8L("Simplify").c_str(), NULL, flag);
    draw_tool();
    ImGui::End();
}

void SimplifyGizmo::draw_tool()
{
    bool is_cancelling = false;
    bool is_worker_running = false;
    bool is_result_ready = false;
    int progress = 0;
    {
        std::lock_guard lk(m_state_mutex);
        is_cancelling = m_state.status == State::cancelling;
        is_worker_running = m_state.status == State::running;
        is_result_ready = !m_state.result.empty();
        progress = m_state.progress;
    }

    bool is_multipart = (m_volume_ids.size() > 1);

    ImGui::Text("%s", (_u8L("mesh name") + ":").c_str());
    ImGui::SameLine();
    ImGui::Text("%s", m_mesh_name.c_str());

    ImGui::Text("%s", (_u8L("triangles") + ":").c_str());
    ImGui::SameLine();
    ImGui::Text("%zu", m_original_triangle_count);

    ImGui::Separator();
        
    // Whether to trigger calculation after rendering is done.
    bool start_process = false;
    if (ImGui::RadioButton("##use_error", !m_configuration.use_count) && !is_multipart) {
        m_configuration.use_count = !m_configuration.use_count;
        start_process = true;
    }
    ImGui::SameLine();

    //m_imgui->disabled_begin(m_configuration.use_count);
    ImGui::Text("%s", _u8L("detail_level").c_str());
    static std::vector<std::string> reduce_captions = {
        _u8L("Extra high"),
        _u8L("High"),
        _u8L("Medium"),
        _u8L("Low"),
        _u8L("Extra low")
    };
    ImGui::SameLine();
    static int reduction = 2;
    if(ImGui::SliderInt("##ReductionLevel", &reduction, 0, 4, reduce_captions[reduction].c_str())) {
        if (reduction < 0) reduction = 0;
        if (reduction > 4) reduction = 4;
        switch (reduction) {
        case 0: m_configuration.max_error = 1e-3f; break;
        case 1: m_configuration.max_error = 1e-2f; break;
        case 2: m_configuration.max_error = 0.1f; break;
        case 3: m_configuration.max_error = 0.5f; break;
        case 4: m_configuration.max_error = 1.f; break;
        }
        start_process = true;
    }
    //m_imgui->disabled_end(); // !use_count

    if (ImGui::RadioButton("##use_count", m_configuration.use_count) && !is_multipart) {
        m_configuration.use_count = !m_configuration.use_count;
        start_process = true;
    } else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && is_multipart)
        ImGui::SetTooltip("%s", "A multipart object can be simplified using only a Level of detail. "
                                     "If you want to enter a Decimate ratio, do the simplification separately.");
    ImGui::SameLine();

    // show preview result triangle count (percent)
    if (!m_configuration.use_count) {
        m_configuration.wanted_count = static_cast<uint32_t>(m_triangle_count);
        m_configuration.decimate_ratio = 
            (1.0f - (m_configuration.wanted_count / (float) m_original_triangle_count)) * 100.f;
    }

    //m_imgui->disabled_begin(!m_configuration.use_count);
    ImGui::Text("decimate_ratio");
    ImGui::SameLine();
    const char * format = (m_configuration.decimate_ratio > 10)? "%.0f %%": 
        ((m_configuration.decimate_ratio > 1)? "%.1f %%":"%.2f %%");
    if (ImGui::SliderFloat("##decimate_ratio", &m_configuration.decimate_ratio, 0.f, 100.f, format)) {
        if (m_configuration.decimate_ratio < 0.f)
            m_configuration.decimate_ratio = 0.01f;
        if (m_configuration.decimate_ratio > 100.f)
            m_configuration.decimate_ratio = 100.f;
        m_configuration.fix_count_by_ratio(m_original_triangle_count);
        start_process = true;
    }

    ImGui::Text(_u8L("%d triangles").c_str(), m_configuration.wanted_count);
    //m_imgui->disabled_end(); // use_count

    if (ImGui::Checkbox(_u8L("Show wireframe").c_str(), &m_show_wireframe)) {
        init_material();
        
        Scene::Scene& scene = m_scene_presenter.scene();
        Node::NodeList simplify_nodes;
        scene.root().query(is_simplify_node, simplify_nodes);
        for (Node* node : simplify_nodes)
            node->set_material_override(m_material);
    }

    //m_imgui->disabled_begin(is_cancelling);
    if (ImGui::Button(_u8L("Close").c_str())) {
        close();
    } else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && is_cancelling)
        ImGui::SetTooltip("%s", _u8L("Operation already cancelling. Please wait few seconds.").c_str());
    //m_imgui->disabled_end(); // state cancelling

    ImGui::SameLine();

    //m_imgui->disabled_begin(is_worker_running || ! is_result_ready);
    if (ImGui::Button(_u8L("Apply").c_str())) {
        apply_simplify();
    } else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && is_worker_running)
        ImGui::SetTooltip("%s", _u8L("Can't apply when proccess preview.").c_str());
    //m_imgui->disabled_end(); // state !settings 

    // draw progress bar
    if (is_worker_running) { // apply or preview
        ImGui::SameLine();
        // draw progress bar
        std::string progress_text = (boost::format(_u8L("Process %1% / 100")) % std::to_string(progress)).str();
        ImGui::ProgressBar(progress / 100., {}, progress_text.c_str());
    }

    if (start_process)
        process();
}

void SimplifyGizmo::on_activated() { }
void SimplifyGizmo::on_deactivated() {
    stop_worker_thread_request();

    // Enable previously disabled node
    for (Node* node : m_to_enable) // TODO: iterate over existing and enable only when exist
        node->set_enabled(true);    // make original volume visible again
    m_to_enable.clear();

    Scene::Scene& scene = m_scene_presenter.scene();
    Node::NodeList simplify_nodes;
    scene.root().query(is_simplify_node, simplify_nodes);

    
    // remove all simplify nodes
    for (Node* node : simplify_nodes)
        scene.remove_children(is_simplify_node, node->parent());

    // Free geometries
    m_phantoms.clear();
    m_volume_ids.clear();
}

void SimplifyGizmo::close(){ m_close_fn(); }
void SimplifyGizmo::apply_simplify()
{
    m_to_enable.clear();
    // worker must be stopped
    assert(m_state.status == State::Status::idle);

    // check that there is NO change of volume
    assert(m_state.volume_ids == m_volume_ids);

    const Project& project = m_project_interactor.selected_project();

    SceneInteractor::RefMeshes meshes;
    meshes.reserve(m_state.result.size());
    for (auto& item : m_state.result) {
        const ObjectID& volume_id = item.first;
        indexed_triangle_set& its = *item.second;

        const ModelVolume* volume = get_volume_by_id(volume_id ,project);
        if (volume == nullptr)
            continue;
        
        size_t object_id = volume->get_object()->id().id;
        ElementRef ref(object_id, 0, volume_id.id);
        using Biz::Algorithms::TriangleMesh::construct;
        meshes.emplace_back(ref, construct(std::move(its)));
    }
    m_state.result.clear();

    SceneInteractor& scene_interactor = m_project_interactor.scene_interactor();
    scene_interactor.change_volume_meshes(std::move(meshes));
    close();
}

void SimplifyGizmo::process()
{
    if (m_volume_ids.empty())
        return;

    // m_volume->mesh().its.indices.empty()
    bool configs_match = false;
    bool result_valid = false;
    bool is_worker_running = false;
    {
        std::lock_guard lk(m_state_mutex);
        configs_match = (m_volume_ids == m_state.volume_ids && m_state.config == m_configuration);
        result_valid = !m_state.result.empty();
        is_worker_running = m_state.status == State::running;
    }

    if ((result_valid || is_worker_running) && configs_match) {
        // Either finished or waiting for result already. Nothing to do.
        return;
    }

    if (is_worker_running && !configs_match) {
        // Worker is running with outdated config. Stop it. It will
        // restart itself when cancellation is done.
        stop_worker_thread_request();
        return;
    }

    if (m_worker.joinable()) {
        // This can happen when process() is called after previous worker terminated,
        // but before the worker_finished callback was called. In this case, just join the thread,
        // the callback will check this and do nothing.
        m_worker.join();
    }

    // Copy configuration that will be used.
    m_state.config = m_configuration;
    m_state.volume_ids = m_volume_ids;
    m_state.status = State::running;

    // Create a copy of current meshes to pass to the worker thread.
    // Using unique_ptr instead of pass-by-value to avoid an extra
    // copy (which would happen when passing to std::thread).
    State::Data its;
    const Project& project = m_project_interactor.selected_project();
    for (const ObjectID& volume_id : m_volume_ids) {
        const ModelVolume* volume = get_volume_by_id(volume_id, project);
        its[volume_id] = std::make_unique<indexed_triangle_set>(volume->mesh().its); // copy
    }

    m_worker = std::thread([this](State::Data&& its) {
        // Checks that the UI thread did not request cancellation, throws if so.
        std::function<void(void)> throw_on_cancel = [this]() {
            std::lock_guard lk(m_state_mutex);
            if (m_state.status == State::cancelling)
                throw SimplifyCanceledException();
        };

        // Called by worker thread, updates progress bar.
        // Using CallAfter so the rerequest function is run in UI thread.
        std::function<void(int)> statusfn = [this](int percent) {
            std::lock_guard lk(m_state_mutex);
            m_state.progress = percent;
            // Redraw the UI to show progress bar.
            PlatformServices::instance().render_request_handler().request_render();
        };

        // Initialize.
        uint32_t triangle_count = 0;
        float max_error = std::numeric_limits<float>::max();
        {
            std::lock_guard lk(m_state_mutex);
            if (m_state.config.use_count)
                triangle_count = m_state.config.wanted_count;
            if (!m_state.config.use_count)
                max_error = m_state.config.max_error;
            m_state.progress = 0;
            m_state.result.clear();
            m_state.status = State::Status::running;
        }

        // Start the actual calculation.
        try {
            for (const auto& it : its) {
                float me = max_error;
                its_quadric_edge_collapse(
                    *it.second, triangle_count, &me, throw_on_cancel, statusfn
                );
            }
        } catch (SimplifyCanceledException&) {
            std::lock_guard lk(m_state_mutex);
            m_state.status = State::idle;
        }

        std::lock_guard lk(m_state_mutex);
        if (m_state.status == State::Status::running) {
            // We were not cancelled, the result is valid.
            m_state.status = State::Status::idle;
            m_state.result = std::move(its);
        }

        // Update UI. Use CallAfter so the function is run on UI thread.
        IMainThreadDispatcher& dispatcher = PlatformServices::instance().main_thread_dispatcher();
        dispatcher.dispatch_on_main_thread_after([this]() { worker_finished(); });
    }, std::move(its));
}

bool SimplifyGizmo::stop_worker_thread_request()
{
    std::lock_guard lk(m_state_mutex);
    if (m_state.status != State::running)
        return false;

    m_state.status = State::Status::cancelling;
    return true;
}

// Following is called from a UI thread when the worker terminates
// worker calls it through a CallAfter.
void SimplifyGizmo::worker_finished()
{
    {
        std::lock_guard lk(m_state_mutex);
        if (m_state.status == State::running) {
            // Someone started the worker again, before this callback
            // was called. Do nothing.
            return;
        }
    }
    if (m_worker.joinable())
        m_worker.join();
    
    const auto& result = m_state.result;
    if (!result.empty())
        update_model(result);

    // rerender the UI to show result.
    PlatformServices::instance().render_request_handler().request_render();

    if (m_state.config != m_configuration || m_state.volume_ids != m_volume_ids) {
        // Settings were changed, restart the worker immediately.
        process();
    }
}

void SimplifyGizmo::create_mesh_name() {
    // NOTE: Need m_volume_ids to be set before calling this function.
    if (m_volume_ids.empty()) {
        m_mesh_name = _u8L("Empty");
        return;
    }

    const Project& project = m_project_interactor.selected_project();

    // set mesh name
    m_mesh_name = "Phantom[cnt" + std::to_string(m_phantoms.size()) + "]:";
    for (const ObjectID& volume_id : m_volume_ids) {
        const ModelVolume* volume_ptr = get_volume_by_id(volume_id, project);
        m_mesh_name += volume_ptr->get_object()->name + "-" + volume_ptr->name + ",";
    }
}

void SimplifyGizmo::set_nodes(const NodeInputs& node_inputs)
{
    m_phantoms.clear();
    m_phantoms.reserve(node_inputs.size());

    int layer_index = int(PlaterSceneLayer::GizmoHandles);

    Scene::Scene& scene = m_scene_presenter.scene(); 
    const Project& project = m_project_interactor.selected_project();
    bool enable_ignored = true;
    for (const NodeInput& node_input: node_inputs) {
        const ObjectID& volume_id = node_input.volume_id;
        Node::NodeList volume_nodes;
        scene.root().query([volume_id](const Node* n) -> bool {
                const SceneNodeTag* tag = n->tag_of_type<SceneNodeTag>();
                return tag != nullptr && 
                    tag->volume_id == volume_id.id;
            }, volume_nodes, enable_ignored);
        ASSERT(!volume_nodes.empty());

        // generate clone of goemetry (copy Node)
        const ModelVolume* volume_ptr = get_volume_by_id(volume_id, project);
        const Transform3d volume_tr = volume_ptr->get_matrix(); 
        const auto &its = volume_ptr->mesh().its;
        Phantom phantom{
            .volume_id = volume_id,
            .geometry = Render::geometry_from_triangle_mesh(m_device, *node_input.its)
        };
        m_phantoms.emplace_back(std::move(phantom));

        for (Node* volume_node : volume_nodes) {
            Node * volume_parent = volume_node->parent();
            ASSERT(!volume_node->enabled()); // make original volume invisible

            SimplifyNodeTag tag{};
            Scene::NodeBuilder builder{scene};
            builder
                .set_debug_name("Simplified volume")
                .set_transform(volume_tr)
                .set_tag(tag)
                .set_mesh(m_phantoms.back().geometry.get(), m_material, layer_index);
            scene.add_child(builder.build().release(), volume_parent);
        }
    }

    // calculate triangle count
    m_triangle_count = 0;
    for (const NodeInput& node_input : node_inputs)
        m_triangle_count += node_input.its->indices.size();
}

void SimplifyGizmo::init_material(){
    if (m_show_wireframe) {
        Scene::Scene& scene = m_scene_presenter.scene();
        const Render::Rect& viewport = scene.camera().viewport();
        float half_w = 0.5f * float(viewport.width);
        float half_h = 0.5f * float(viewport.height);
        Matrix4f viewport_matrix;
        viewport_matrix << 
            half_w, 0.0f,   0.0f, half_w,
            0.0f,   half_h, 0.0f, half_h, 
            0.0f,   0.0f,   1.0f, 0.0f,
            0.0f,   0.0f,   0.0f, 1.0f;
        m_material = Render::Material{}
            .set_shader(m_device.context().shader_manager().shader("gouraud_light_wireframe"))
            .set_uniform("wireframe_color", wireframe_color)
            .set_uniform("wireframe_width", wireframe_width)
            .set_uniform("viewport_matrix", viewport_matrix)
            .set_transparent(false);
    } else {
        m_material = Render::Material{}
            .set_shader(m_device.context().shader_manager().shader("gouraud_light"))
            .set_transparent(false);    
    }
}

void SimplifyGizmo::init_model(const std::set<ObjectID>& current_volume_ids)
{
    m_volume_ids = std::move(current_volume_ids);
    NodeInputs node_inputs;
    node_inputs.reserve(m_volume_ids.size());

    Scene::Scene& scene = m_scene_presenter.scene();
    const Project& project = m_project_interactor.selected_project(); 
    for (const ObjectID& volume_id : m_volume_ids) {
        // generate clone of goemetry (copy Node)
        const ModelVolume* volume_ptr = get_volume_by_id(volume_id, project);
        node_inputs.push_back(NodeInput{volume_id, &volume_ptr->mesh().its});
        Node::NodeList volume_nodes;
        scene.root().query([volume_id](const Node* n) -> bool {
                const SceneNodeTag* tag = n->tag_of_type<SceneNodeTag>();
                return tag != nullptr && tag->volume_id == volume_id.id;
            }, volume_nodes);
        ASSERT(!volume_nodes.empty());
        for (Node* volume_node : volume_nodes) {
            ASSERT(volume_node->enabled());
            volume_node->set_enabled(false); // make original volume invisible
            m_to_enable.push_back(volume_node); // save for later enable
        }
    }

    set_nodes(node_inputs);
    create_mesh_name();

    // triangle count is calculated in set_nodes
    m_original_triangle_count = m_triangle_count;

    // Default value of configuration
    m_configuration.decimate_ratio = 50.; // default value
    m_configuration.fix_count_by_ratio(m_original_triangle_count);
    m_configuration.use_count = false;    
    m_configuration.wanted_count = static_cast<uint32_t>(m_triangle_count);
    m_configuration.decimate_ratio =
            (1.0f - (m_configuration.wanted_count / (float) m_original_triangle_count)) * 100.f;
    
}

void SimplifyGizmo::update_model(const State::Data& data)
{
    // check that model exist -> deactivation clear m_phantoms
    ASSERT(!m_phantoms.empty());
        
    // remove all simplify nodes
    Scene::Scene& scene = m_scene_presenter.scene();
    Node::NodeList simplify_nodes;
    scene.root().query(is_simplify_node, simplify_nodes);
    for (Node* node : simplify_nodes)
        scene.remove_children(is_simplify_node, node->parent());

    // convert Data to NodeInputs
    NodeInputs node_inputs;
    node_inputs.reserve(data.size());
    for (const auto& item : data) {
        const ObjectID& volume_id = item.first;
        const indexed_triangle_set& its = *item.second;
        node_inputs.push_back(NodeInput{volume_id, &its});
    }    

    // Recreate simplify nodes
    set_nodes(node_inputs);
}

/////////////////
/// SimplifyGizmo::Configuration
///////////////// 

void SimplifyGizmo::Configuration::fix_count_by_ratio(size_t triangle_count)
{
    if (decimate_ratio <= 0.f)
        wanted_count = static_cast<uint32_t>(triangle_count);
    else if (decimate_ratio >= 100.f)
        wanted_count = 0;
    else
        wanted_count = static_cast<uint32_t>(
            std::round(triangle_count * (100.f - decimate_ratio) / 100.f)
        );
}

bool SimplifyGizmo::Configuration::operator==(const Configuration& rhs)
{
    return 
        use_count == rhs.use_count && 
        decimate_ratio == rhs.decimate_ratio &&
        wanted_count == rhs.wanted_count && 
        max_error == rhs.max_error ;
}

bool SimplifyGizmo::Configuration::operator!=(const Configuration& rhs) {
    return !(*this == rhs);
}

} // namespace Slic3r::App::Plater
