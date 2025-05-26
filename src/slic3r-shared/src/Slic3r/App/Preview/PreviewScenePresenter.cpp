#include "Slic3r/App/Preview/PreviewScenePresenter.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/App/Scene/BedNodeBuilder.hpp"
#include "Slic3r/App/Preview/PreviewSceneLayer.hpp"

namespace Slic3r::App::Preview {

PreviewScenePresenter::PreviewScenePresenter(const Domain::Workbench& workbench,
    Biz::ProjectInteractor& project_interactor, Render::Device& device)
    : m_workbench(workbench), m_project_interactor(project_interactor), m_device(device)
    , m_bed_render_updater(*this, workbench, device)
{
    size_t project_id = m_project_interactor.selected_project_id();
    on_selected_project_changed(project_id);
    m_project_interactor.add_listener<Biz::ISelectedProjectChangedListener>(&m_bed_render_updater);
}

void PreviewScenePresenter::render_scene(Render::CommandBuffer& command_buffer)
{
    if (!m_projects.empty()){
        Scene::SceneRenderFlag flags = Scene::SceneRenderFlag(
            uint32_t(Scene::SceneRenderFlag::Shadows) |
            uint32_t(Scene::SceneRenderFlag::AmbientOcclusion)
        );
        project_context().scene().render(m_device, command_buffer, this, flags);
    }
}

void PreviewScenePresenter::render_imgui(const Render::ScreenInfo& screen_info)
{
    if (!m_projects.empty())
        project_context().scene().render_imgui(screen_info);
}

void PreviewScenePresenter::screen_resized(const Render::Rect& viewport)
{
    update_cameras([&viewport](auto& cam) { cam.set_viewport(viewport); });
}

void PreviewScenePresenter::on_selected_project_changed(size_t index)
{
    m_selected_project_id = index;
    if (m_projects.count(m_selected_project_id) == 0) {
        m_projects.try_emplace(index);
        m_bed_render_updater.on_selected_project_changed(m_selected_project_id);
        project_context().scene().camera().add_listener<Scene::ICameraUpdateListener>(&m_bed_render_updater);
    }
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
        Scene::BedNodeBuilder::bed_node(builder, inst, tag, m_device,
            m_projects[m_selected_project_id], int(PreviewSceneLayer::Bed));

        scn.add_child(builder.build().release());
    }
}

void PreviewScenePresenter::update_bed_instances()
{
    m_bed_render_updater.update_all(scene().camera());

    // update visibility of bed instances
    visit(scene().root(), [&](Scene::Node& n) {
        Scene::BedNodeTag* tag = n.tag_of_type<Scene::BedNodeTag>();
        if (tag != nullptr && tag->type == Scene::BedElementType::Undefined) {
            const auto& proj = m_workbench.project(m_selected_project_id);
            const Domain::ConfigContainer* cc = proj.find_config_container(tag->config_container_id);
            const Domain::BedInstance* inst = Domain::find_by_id(cc->bed_instances(), tag->instance_id);
            if (inst == nullptr)
                return;
            n.set_enabled(inst->active);
        }
    }, true);
}

void PreviewScenePresenter::update_cameras(const std::function<void(Scene::Camera&)>& modifier)
{
    std::for_each(m_projects.begin(), m_projects.end(), [modifier](auto& p) {
        modifier(p.second.scene().camera());
    });
}

} // namespace Slic3r::App::Preview
