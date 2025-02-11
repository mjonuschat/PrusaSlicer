#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"

#include <libslic3r/Model.hpp>

#include <Slic3r/Assert.hpp>

namespace Slic3r::Biz::Scene {

static const Vec2d BED_GAP = { 20.0, 20.0 };

namespace {

Geometry::Transformation transform_product(const Geometry::Transformation& orig_xform, const SceneInteractor::Transform& delta)
{
    Transform3d xform = orig_xform.get_matrix();
    xform = delta * xform.matrix();
    return Geometry::Transformation{xform};
}

Geometry::Transformation transform_product(const  SceneInteractor::Transform& orig_xform, const SceneInteractor::Transform& delta)
{
    SceneInteractor::Transform xform = delta * orig_xform;
    return Geometry::Transformation{Transform3d {xform}};
}

void transform_selection_instance_mode(
    const SceneInteractorProjectContext& proj,
    const SceneInteractor::Transform& relative_transform,
    TransformMemento& memento
)
{
    const bool initialize_memento = memento.elements.empty();
    const auto& sel = proj.selection;
    DEBUG_ASSERT(sel.mode == SelectionMode::Instance);

    if (initialize_memento)
        memento.elements.reserve(sel.elements.size());
    for (const auto& e : sel.elements) {
        auto* inst = proj.project.find_instance_by_id(e.object_id, e.instance_id);
        if (initialize_memento)
            memento.elements.insert({e, {e, inst->get_matrix().matrix()}});
        inst->set_transformation(transform_product(memento.elements[e].original_xform, relative_transform));
    }
}

void transform_selection_volume_mode(
    const SceneInteractorProjectContext& proj,
    const SceneInteractor::Transform& relative_transform,
    TransformMemento& memento
)
{
    const bool initialize_memento = memento.elements.empty();
    const auto& sel = proj.selection;
    DEBUG_ASSERT(sel.mode == SelectionMode::Volume);

    if (initialize_memento)
        memento.elements.reserve(sel.elements.size());
    for (const auto& e : sel.elements) {
        DEBUG_ASSERT(e.volume_id != 0);
        auto* vol = proj.project.find_volume_by_id(e.object_id, e.volume_id);
        if (initialize_memento)
            memento.elements.insert({e, {e, vol->get_matrix().matrix()}});
        vol->set_transformation(transform_product(memento.elements[e].original_xform, relative_transform));
    }
}

} // namespace

void SceneInteractor::on_selected_project_changed(size_t index)
{
    auto& project = m_workbench.project(index);
    if (m_projects.count(index) == 0)
        m_projects.emplace(index, SceneInteractorProjectContext{project});
    m_selected_project_id = index;

}

void SceneInteractor::on_selected_config_container_changed(Domain::SelectionId project_id,
    Domain::SelectionId container_id)
{
    DEBUG_ASSERT(project_id == m_selected_project_id);
    m_selected_config_container_id = container_id;
}

const Selection& SceneInteractor::selection() const
{
    ASSERT(m_selected_project_id != Domain::INVALID_ID);
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());
    return it->second.selection;
}

void SceneInteractor::set_selection(const Selection& selection)
{
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());
    DEBUG_ASSERT(selection.is_valid());
    it->second.selection = selection;
    m_selection_changed_listeners.invoke([&](auto* l){
        l->on_scene_selection_changed(m_selected_project_id, selection);
    });
}

void SceneInteractor::modify_selection(const std::function<void(Selection&)>& modifier)
{
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());
    auto& selection = it->second.selection;
    modifier(selection);
    DEBUG_ASSERT(selection.is_valid());
    m_selection_changed_listeners.invoke([&](auto* l){
        l->on_scene_selection_changed(m_selected_project_id, selection);
    });

}

void SceneInteractor::new_object_from_mesh(TriangleMesh&& mesh)
{
    auto& project = m_workbench.project(m_selected_project_id);
    auto& obj = *project.model().add_object();
    auto& vol = *obj.add_volume(std::move(mesh));
    auto& inst = *obj.add_instance();
    const Domain::ElementRefs updated {{obj.id().id, inst.id().id, 0}};
    project.update_instances_bed_placement(updated);

    m_changed_listeners.invoke([&](auto* l) {
        l->on_instance_added(m_selected_project_id, updated);
    });

    set_selection({SelectionMode::Instance, {updated}});
}

void SceneInteractor::add_volume_from_mesh(TriangleMesh&& mesh, ModelVolumeType volume_type, const Transform& xform)
{
    auto& project = m_workbench.project(m_selected_project_id);
    const Selection& sel = selection();
    //DEBUG_ASSERT(sel.mode == SelectionMode::Instance);
    DEBUG_ASSERT(sel.elements.size() == 1);
    size_t obj_id = sel.elements[0].object_id;
    Domain::ElementRefs updated;
    
    auto& obj = *project.find_object_by_id(obj_id);
    auto& vol = *obj.add_volume(std::move(mesh), volume_type);
    vol.set_transformation(Transform3d{xform});
    updated.push_back({obj.id().id, obj.instances[0]->id().id, vol.id().id});


    m_changed_listeners.invoke([&](auto* l) {
        l->on_volume_added(m_selected_project_id, updated);
    });

    set_selection({SelectionMode::Volume, updated});
}

void SceneInteractor::add_instance(const Transform& xform)
{
    auto& project = m_workbench.project(m_selected_project_id);
    const Selection& sel = selection();
    //DEBUG_ASSERT(sel.mode == SelectionMode::Instance);
    DEBUG_ASSERT(sel.elements.size() == 1);
    size_t obj_id = sel.elements[0].object_id;
    Domain::ElementRefs updated;

    auto& obj = *project.find_object_by_id(obj_id);
    auto& inst = *obj.add_instance();
    inst.set_transformation(Geometry::Transformation{Transform3d{xform}});
    updated.push_back({obj.id().id, inst.id().id, 0});

    project.update_instances_bed_placement(updated);

    m_changed_listeners.invoke([&](auto* l) {
        l->on_instance_added(m_selected_project_id, updated);
    });

    set_selection({SelectionMode::Instance, updated});

}

Domain::BedInstance& SceneInteractor::add_bed_instance(size_t config_container_id)
{
    auto& project = m_projects.find(m_selected_project_id)->second.project;
    Domain::ConfigContainer* cc = project.find_config_container(config_container_id);
    Domain::BedInstance& ret = cc->add_bed_instance();

    m_bed_placement.layout(project, BED_GAP);

    // make copy
    Domain::ModelInstanceList unplaced = project.unplaced_model_instances();
    project.unplaced_model_instances().clear();
    project.update_instances_bed_placement(unplaced, false);

    const Domain::BedRef updated{ cc->id().id, ret.id().id };
    m_changed_listeners.invoke([&](auto* l) {
        l->on_bed_instance_added(m_selected_project_id, { updated });
    });
    return ret;
}

void SceneInteractor::remove_bed_instance(const Domain::BedRef& instance)
{
    auto& project = m_projects.find(m_selected_project_id)->second.project;
    Domain::ConfigContainer* cc = project.find_config_container(instance.config_container_id);
    DEBUG_ASSERT(cc != nullptr);
    auto* bed_inst = Domain::find_by_id(cc->bed_instances(), instance.instance_id);

    auto& insts = bed_inst->model_instances();
    auto& unplaced = project.unplaced_model_instances();
    unplaced.insert(unplaced.end(), insts.begin(), insts.end());

    cc->remove_bed_instance_by_id(instance.instance_id);

    m_bed_placement.layout(project, BED_GAP);

    m_changed_listeners.invoke([&](auto* l) {
        l->on_bed_instance_removed(m_selected_project_id, { instance });
    });
}

void SceneInteractor::transform_bed_instance(const Domain::BedRef& instance, const Transform& xform)
{
    auto& proj = m_projects.find(m_selected_project_id)->second;
    Domain::ConfigContainer* cc = proj.project
        .find_config_container(instance.config_container_id);
    Domain::BedInstance& inst = cc->find_bed_instance(instance.instance_id);
    inst.set_transformation(Geometry::Transformation{ Transform3d{xform} });

    auto updated = inst.model_instances();
    inst.model_instances().clear();
    proj.project.update_instances_bed_placement(updated, false);

    m_changed_listeners.invoke([&](auto* l) {
        l->on_bed_instance_transformed(m_selected_project_id, { instance });
    });
}

void SceneInteractor::select_bed_instance(const Domain::BedRef& instance)
{
    if (instance == m_selected_bed_instance)
        return;

    Domain::Project::ConfigContainerList& ccs = m_projects.find(m_selected_project_id)->second.project.config_containers();
    for (auto& cc : ccs) {        
        Domain::ConfigContainer::BedInstanceList& instances = cc->bed_instances();
        for (auto& inst : instances) {
            inst->set_active(cc->id().id == instance.config_container_id && inst->id().id == instance.instance_id);
        }
    }

    m_selected_bed_instance = instance;

    m_bed_instance_selection_changed_listeners.invoke([&](auto* l) {
        l->on_selected_bed_instance_changed(
            m_selected_project_id, instance.config_container_id, instance.instance_id);
    });
}

void SceneInteractor::select_first_bed_instance()
{
    const auto& cc = m_projects.find(m_selected_project_id)->second.project.config_containers().front();
    select_bed_instance({ cc->id().id, cc->bed_instances().front()->id().id });
}

void SceneInteractor::transform_selection(const Matrix4d& relative_transform, TransformMemento& memento)
{
    auto& proj = m_projects.find(m_selected_project_id)->second;
    const bool instance_mode = proj.selection.mode == SelectionMode::Instance;
    if (instance_mode)
        transform_selection_instance_mode(proj, relative_transform, memento);
    else
        transform_selection_volume_mode(proj, relative_transform, memento);
    update_selection_instance_bed_placement();
    m_changed_listeners.invoke([&](ISceneChangedListener* l) {
        if (instance_mode)
            l->on_instance_transformed(m_selected_project_id, proj.selection.elements);
        else
            l->on_volume_transformed(m_selected_project_id, proj.selection.elements);
    });
    m_selection_changed_listeners.invoke([&](ISceneSelectionChangedListener* l) {
        l->on_scene_selection_transformed(m_selected_project_id, proj.selection);
    });
}

void SceneInteractor::finalize_transform_selection(TransformMemento& memento, bool canceled)
{
    if (!canceled) {
        memento.reset();
        return;
    }

    auto& proj = m_projects.find(m_selected_project_id)->second;
    const bool vol_mode = proj.selection.mode == SelectionMode::Volume;
    for (const auto& [_, e] : memento.elements) {
        const Geometry::Transformation xform{Transform3d {e.original_xform}};
        if (vol_mode) {
            auto* vol = proj.project.find_volume_by_id(e.element.object_id, e.element.volume_id);
            vol->set_transformation(xform);
        } else {
            auto* inst = proj.project.find_instance_by_id(e.element.object_id, e.element.instance_id);
            inst->set_transformation(xform);
        }
    }

    update_selection_instance_bed_placement();

    m_changed_listeners.invoke([&](ISceneChangedListener* l) {
        if (vol_mode)
            l->on_volume_transformed(m_selected_project_id, proj.selection.elements);
        else
            l->on_instance_transformed(m_selected_project_id, proj.selection.elements);
    });
    memento.reset();
}

void SceneInteractor::update_selection_instance_bed_placement()
{
    auto& proj = m_projects.find(m_selected_project_id)->second;
    const bool vol_mode = proj.selection.mode == SelectionMode::Volume;
    if (vol_mode) {
        std::set<size_t> object_ids;
        for (const auto& e : proj.selection.elements)
            object_ids.insert(e.object_id);
        for (size_t obj_id : object_ids)
            proj.project.update_instances_bed_placement(proj.project.find_object_by_id(obj_id)->instances);
    } else {
        proj.project.update_instances_bed_placement(proj.selection.elements);
    }
}

} // namespace Slic3r::Biz::Scene
