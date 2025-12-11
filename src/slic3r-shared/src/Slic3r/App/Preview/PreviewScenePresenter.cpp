#include "Slic3r/App/Preview/PreviewScenePresenter.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/App/Scene/BedNodeBuilder.hpp"
#include "Slic3r/App/Preview/PreviewSceneLayer.hpp"
#include "Slic3r/App/Scene/CameraHelper.hpp"

namespace Slic3r::App::Preview {

PreviewScenePresenter::PreviewScenePresenter(
    const Domain::Workbench& workbench,
    Biz::ProjectInteractor& project_interactor,
    Render::Device& device,
    Platform::AnimationManager& animation_manager
) :
    m_workbench(workbench),
    m_project_interactor(project_interactor),
    m_device(device),
    m_animation_manager(animation_manager),
    m_bed_render_updater(*this, workbench, device, project_interactor.scene_interactor())
{
    m_project_interactor.add_listener<Biz::ISelectedProjectChangedListener>(this);
    m_project_interactor.add_listener<Biz::ISelectedProjectChangedListener>(&m_bed_render_updater);

    size_t project_id = m_project_interactor.selected_project_id();
    on_selected_project_changed(project_id);
}

void PreviewScenePresenter::render_scene(Render::CommandBuffer& command_buffer)
{
    if (!m_projects.empty()) {
#if ENABLE_DEBUG_RENDER_SCENE_AABB
        m_camera_frustum_updater.update_scene_aabb(project_context());
        m_camera_frustum_updater.update_scene_aabb_node(project_context(), m_device);
#else
        m_camera_frustum_updater.update_scene_aabb(scene());
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB
        m_camera_frustum_updater.update_camera_frustum(scene().camera());

        project_context().scene().render(m_device, command_buffer, this);
    }
}

void PreviewScenePresenter::render_imgui(const Render::ScreenInfo& screen_info)
{
    if (!m_projects.empty()) {
#if ENABLE_DEBUG_BED_ERROR
        render_imgui_debug_bed_error(project_context().bed_error());
#endif // ENABLE_DEBUG_BED_ERROR
        project_context().scene().render_imgui(screen_info);
    }
}

void PreviewScenePresenter::screen_resized(const Render::Rect& viewport)
{
    m_viewport = viewport;
    update_cameras([&viewport](auto& cam) { cam.set_viewport(viewport); });
}

void PreviewScenePresenter::on_selected_project_changed(size_t index)
{
    m_selected_project_id = index;
    if (m_projects.count(m_selected_project_id) == 0) {
        m_projects.try_emplace(index);
        m_bed_render_updater.on_selected_project_changed(m_selected_project_id);
        // a new camera has been created, add the camera update listeners
        auto& camera = project_context().scene().camera();
        camera.add_listener<Scene::ICameraUpdateListener>(&m_bed_render_updater);
        camera.add_listener<Scene::ICameraUpdateListener>(this);
        camera.set_viewport(m_viewport);
        project_context().scene().add_listener<Scene::ISceneChangedListener>(this);
    }
    set_scene_aabb_as_dirty();
}

void PreviewScenePresenter::on_node_added(Scene::Node* node)
{
    if (node != nullptr && node->contains_raycast_component())
        set_scene_aabb_as_dirty();
}

void PreviewScenePresenter::on_node_removed(Scene::Node* node)
{
    if (node != nullptr && node->contains_raycast_component())
        set_scene_aabb_as_dirty();
}

void PreviewScenePresenter::on_node_changed(Scene::Node* node)
{
    if (node != nullptr && node->contains_raycast_component())
        set_scene_aabb_as_dirty();
}

void PreviewScenePresenter::remove_all_bed_instances()
{
    scene().remove_children([&](const Scene::Node* n) {
        return n->tag_of_type<Scene::BedNodeTag>() != nullptr;
    });  
}

void PreviewScenePresenter::add_bed_instances(const Domain::BedRefs& instances)
{
    auto& scn = scene();
    const auto& proj = m_workbench.project(m_selected_project_id);
    
    for (auto& instance : instances) {
        const Domain::ConfigContainer* cc =
            proj.find_config_container(instance.config_container_id); DEBUG_ASSERT(cc != nullptr);
        const Domain::BedInstance& inst = cc->find_bed_instance(instance.instance_id);

        Scene::BedNodeTag tag = { instance.config_container_id, instance.instance_id };

        Scene::NodeBuilder builder(scn);
        Scene::build_bed_node(builder, inst, tag, m_device,
            m_projects[m_selected_project_id], Scene::RenderLayerId(PreviewSceneLayer::Bed));

        scn.add_child(builder.build().release());
    }
}

void PreviewScenePresenter::update_bed_instances()
{
    m_bed_render_updater.update_all(scene().camera(), project_context().bed_error());
    const auto& scene_interactor = m_project_interactor.scene_interactor();
    const Biz::Scene::BedSelection selection{scene_interactor.bed_selection()};

    // update visibility of bed instances
    visit(
        scene().root(),
        [&](Scene::Node& n) {
            Scene::BedNodeTag* tag = n.tag_of_type<Scene::BedNodeTag>();
            if (tag != nullptr && tag->type == Scene::BedElementType::Undefined) {
                const auto& proj                  = m_workbench.project(m_selected_project_id);
                const Domain::ConfigContainer* cc = proj.find_config_container(
                    tag->config_container_id
                );
                const Domain::BedInstance* inst = Domain::find_by_id(
                    cc->bed_instances(),
                    tag->instance_id
                );
                if (inst == nullptr)
                    return;

                const Domain::BedRef bed_ref{cc->id().id, inst->id().id};
                n.set_enabled(selection.last_selected_bed() == bed_ref);
            }
        },
        true
    );
}

bool PreviewScenePresenter::update_bed_instance_error_state(const Domain::SlicingId& id, bool error)
{
    bool ret = error ? project_context().bed_error().add_bed_instance(id) : project_context().bed_error().remove_bed_instance(id);
    if (ret)
        update_bed_instances();
    return ret;
}

void PreviewScenePresenter::center_camera_on_selected_bed(bool animated)
{
    if (animated)
        animated_center_camera_on_bed(m_workbench.project(m_project_interactor.selected_project_id()),
            m_project_interactor.scene_interactor().bed_selection().last_selected_bed(), scene().camera_trackball(),
            m_animation_manager);
    else
        center_camera_on_bed(m_workbench.project(m_project_interactor.selected_project_id()),
            m_project_interactor.scene_interactor().bed_selection().last_selected_bed(), scene().camera_trackball());
}

void PreviewScenePresenter::update_cameras(const std::function<void(Scene::Camera&)>& modifier)
{
    std::for_each(m_projects.begin(), m_projects.end(), [modifier](auto& p) {
        modifier(p.second.scene().camera());
    });
}

} // namespace Slic3r::App::Preview
