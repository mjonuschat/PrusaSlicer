#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "libslic3r/Model.hpp"

namespace Slic3r::Biz::Scene {

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

    project.bed_container().reset();
    Domain::Bed* bed = project.bed_container().add_bed(
        project.config_containers().front()->get_print_config(),
        m_workbench.preset_bundle());

    // temporary member to allow to select a bed from ConfigContainer until we do not have a mechanism for it
    m_bed_id = bed->id().id;
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

    m_changed_listeners.invoke([&](auto* l) {
        l->on_instance_added(m_selected_project_id, updated);
    });

    set_selection({SelectionMode::Instance, updated});

}

void SceneInteractor::new_bed(size_t idx, const Transform& xform)
{
    Domain::Bed* bed = m_workbench.project(m_selected_project_id).bed_container().bed(idx);
    DEBUG_ASSERT(bed != nullptr);
    DEBUG_ASSERT(bed->instances().empty());
    add_bed_instance(idx, xform);
}

void SceneInteractor::add_bed_instance(size_t idx, const Transform& xform)
{
    Domain::Bed* bed = m_workbench.project(m_selected_project_id).bed_container().bed(idx);
    DEBUG_ASSERT(bed != nullptr);
    Domain::BedInstance* inst = bed->add_instance();
    DEBUG_ASSERT(inst != nullptr);
    inst->set_transformation(Geometry::Transformation{ Transform3d{xform} });
    const Domain::ElementRef updated{ bed->id().id, inst->id().id };

    m_changed_listeners.invoke([&](auto* l) {
        l->on_bed_instance_added(m_selected_project_id, updated);
    });
}

void SceneInteractor::transform_bed_instance(const Domain::ElementRef& instance, const Transform& xform)
{
    Domain::Bed* bed = m_workbench.project(m_selected_project_id).bed_container().bed(instance.object_id);
    DEBUG_ASSERT(bed != nullptr);
    Domain::BedInstance* inst = bed->instance(instance.instance_id);
    DEBUG_ASSERT(inst != nullptr);
    inst->set_transformation(Geometry::Transformation{ Transform3d{xform} });
    const Domain::ElementRef updated{ bed->id().id, inst->id().id };

    m_changed_listeners.invoke([&](auto* l) {
        l->on_bed_instance_transformed(m_selected_project_id, updated);
    });
}

// temporary method to allow to select a bed from ConfigContainer until we do not have a mechanism for it
Domain::Bed* SceneInteractor::bed()
{
    return m_workbench.project(m_selected_project_id).bed_container().bed(m_bed_id);
}

void SceneInteractor::transform_selection(const Matrix4d& relative_transform, TransformMemento& memento)
{
    auto& proj = m_projects.find(m_selected_project_id)->second;
    const bool instance_mode = proj.selection.mode == SelectionMode::Instance;
    if (instance_mode)
        transform_selection_instance_mode(proj, relative_transform, memento);
    else
        transform_selection_volume_mode(proj, relative_transform, memento);
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

    m_changed_listeners.invoke([&](ISceneChangedListener* l) {
        if (vol_mode)
            l->on_volume_transformed(m_selected_project_id, proj.selection.elements);
        else
            l->on_instance_transformed(m_selected_project_id, proj.selection.elements);
    });
    memento.reset();
}

} // namespace Slic3r::Biz::Scene
