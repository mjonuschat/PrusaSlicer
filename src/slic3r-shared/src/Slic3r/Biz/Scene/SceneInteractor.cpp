#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/Scene/BedTracking.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"

#include <libslic3r/Model.hpp>

#include <Slic3r/Assert.hpp>
#include <vector>
#include <algorithm>

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
    invoke_listeners<ISceneSelectionChangedListener>([&](auto* l){
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
    invoke_listeners<ISceneSelectionChangedListener>([&](auto* l){
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
    auto changes = update_instances_bed_placement(project, updated);

    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);
    invoke_listeners<ISceneChangedListener>([&](auto* l) {
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

    invoke_listeners<ISceneChangedListener>([&](auto* l) {
        l->on_volume_added(m_selected_project_id, updated);
    });

    set_selection({SelectionMode::Volume, updated});
    update_selection_instance_bed_placement();
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

    auto changes = update_instances_bed_placement(project, updated);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    invoke_listeners<ISceneChangedListener>([&](auto* l) {
        l->on_instance_added(m_selected_project_id, updated);
    });

    set_selection({SelectionMode::Instance, updated});
}

void SceneInteractor::notify_listener_on_objects(const Slic3r::ModelObjectPtrs& objects)
{
    auto& project = m_workbench.project(m_selected_project_id);
    for (const Slic3r::ModelObject* object : objects) {
        Domain::ElementRefs updated;

        for (const Slic3r::ModelInstance* inst : object->instances)
            updated.push_back({ object->id().id, inst->id().id });

        auto changes = update_instances_bed_placement(project, updated);
        for (const auto& bed_ref : changes.updated_beds)
            invoke_slicing_input_changed(bed_ref);

        invoke_listeners<ISceneChangedListener>([&](auto* l) {
            l->on_instance_added(m_selected_project_id, updated);
        });

        Domain::ElementRefs updated_vols;
        for (const Slic3r::ModelVolume* vol : object->volumes)
            updated_vols.push_back({ object->id().id, object->instances[0]->id().id, vol->id().id });
        invoke_listeners<ISceneChangedListener>([&](auto* l) {
            l->on_volume_added(m_selected_project_id, updated_vols);
        });
    }
}

void SceneInteractor::notify_listener_on_objects()
{
    auto& project = m_workbench.project(m_selected_project_id);
    notify_listener_on_objects(project.model().objects);
}

void SceneInteractor::edit_name(const Domain::ElementRef& id, const std::string& new_name)
{
    Domain::Project& project = m_workbench.project(m_selected_project_id);
    if (id.volume_id == 0)
        project.find_object_by_id(id.object_id)->name = new_name;
    else
        project.find_volume_by_id(id.object_id, id.volume_id)->name = new_name;
}

void SceneInteractor::set_printable(const Domain::ElementRef& id, bool is_printable)
{
    assert(id.volume_id == 0);
    Domain::Project& project = m_workbench.project(m_selected_project_id);
    if (id.instance_id == 0)
        project.find_object_by_id(id.object_id)->printable = is_printable;
    else
        project.find_instance_by_id(id.object_id, id.instance_id)->printable = is_printable;
}

void SceneInteractor::extract_selected_instances()
{
    const Selection& scene_selection = selection();
    if (scene_selection.empty() || scene_selection.mode != SelectionMode::Instance)
        return;

    bool all_instances_from_one_object = true;
    size_t object_id = scene_selection.elements[0].object_id;
    for (const auto& el : scene_selection.elements) {
        if (object_id != el.object_id) {
            all_instances_from_one_object = false;
            break;
        }
    }
    ASSERT(all_instances_from_one_object);

    Domain::Project& project = m_workbench.project(m_selected_project_id);
    Slic3r::Model&   model   = project.model();

    Selection::ElementRefs to_remove = scene_selection.elements;
    ModelObjectPtrs        new_objects;
    ModelObject*           old_object = project.find_object_by_id(object_id);
    size_t                 sel_object_id = old_object->id().id;

    if (old_object->instances.size() == to_remove.size()) {
        // splite old_object instances into separate object
        
        for (int inst_cnt = int(old_object->instances.size()) - 1; inst_cnt > 0; inst_cnt--) {
            // make a copy of the active object
            Slic3r::ModelObject* new_object = model.add_object(*old_object);
            new_objects.emplace_back(new_object);
            // delete no needed instances from new_object
            for (size_t idx = old_object->instances.size() - 1; idx != size_t(-1); idx--) {
                if (inst_cnt != idx)
                    new_object->delete_instance(idx);
            }
        }
        // delete no needed instances from old_object
        for (size_t idx = old_object->instances.size() - 1; idx > 0; idx--)
            old_object->delete_instance(idx);

        Domain::ElementRef stay_el({ sel_object_id, old_object->instances[0]->id().id });
        to_remove.erase(std::remove_if(to_remove.begin(), to_remove.end(), [stay_el](const Domain::ElementRef& el) { return el == stay_el; }), to_remove.end());
    }
    else {
        //extract selected instances into separate object

        // make a copy of the active object
        Slic3r::ModelObject* new_object = model.add_object(*old_object);
        new_objects.emplace_back(new_object);

        // delete no needed instances from both objects
        for (size_t idx = old_object->instances.size() - 1; idx != size_t(-1); idx--) {
            if (scene_selection.is_selected({ sel_object_id, old_object->instances[idx]->id().id }))
                old_object->delete_instance(idx);
            else
                new_object->delete_instance(idx);
        }
    }

    // notify listener on chnages

    notify_listener_on_objects(new_objects);
    invoke_listeners<ISceneChangedListener>([&](auto* l) {
        l->on_instance_removed(m_selected_project_id, to_remove);
    });
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
    auto changes = update_instances_bed_placement(project, unplaced, false);
    const Domain::BedRef updated{ cc->id().id, ret.id().id };
    changes.updated_beds.insert(updated);

    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    invoke_listeners<ISceneChangedListener>([&](auto* l) {
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

    auto insts = bed_inst->model_instances();

    cc->remove_bed_instance_by_id(instance.instance_id);
    m_bed_placement.layout(project, BED_GAP);
    auto changes = update_instances_bed_placement(project, insts);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    invoke_listeners<ISlicingInputChangedListener>([&](auto* l) {
        l->on_slicing_input_removed(instance);
    });
    invoke_listeners<ISceneChangedListener>([&](auto* l) {
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
    auto changes = update_instances_bed_placement(proj.project, updated, false);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    invoke_listeners<ISceneChangedListener>([&](auto* l) {
        l->on_bed_instance_transformed(m_selected_project_id, { instance });
    });
}

void SceneInteractor::select_bed_instance(const Domain::BedRef& instance)
{
    select_bed_instance_internal(instance, false);
}

void SceneInteractor::select_first_bed_instance()
{
    const auto& cc = m_projects.find(m_selected_project_id)->second.project.config_containers().front();
    select_bed_instance_internal({ cc->id().id, cc->bed_instances().front()->id().id }, true);
}

const Domain::Project::ConfigContainerList& SceneInteractor::selected_project_config_containers()
{
    return m_projects.find(m_selected_project_id)->second.project.config_containers();
}

const Domain::ModelInstanceList& SceneInteractor::selected_project_unplaced_model_instances()
{
    return m_projects.find(m_selected_project_id)->second.project.unplaced_model_instances();
}

void SceneInteractor::transform_selection(const Transform& relative_transform)
{
    TransformMemento memento;
    transform_selection(relative_transform, memento);
    finalize_transform_selection(memento, false);
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
    invoke_listeners<ISceneChangedListener>([&](ISceneChangedListener* l) {
        if (instance_mode)
            l->on_instance_transformed(m_selected_project_id, proj.selection.elements);
        else
            l->on_volume_transformed(m_selected_project_id, proj.selection.elements);
    });
    invoke_listeners<ISceneSelectionChangedListener>([&](ISceneSelectionChangedListener* l) {
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

    invoke_listeners<ISceneChangedListener>([&](ISceneChangedListener* l) {
        if (vol_mode)
            l->on_volume_transformed(m_selected_project_id, proj.selection.elements);
        else
            l->on_instance_transformed(m_selected_project_id, proj.selection.elements);
    });
    memento.reset();
}

void SceneInteractor::update_selection_instance_bed_placement()
{
    BedTrackingChanges changes;
    auto& proj = m_projects.find(m_selected_project_id)->second;
    const bool vol_mode = proj.selection.mode == SelectionMode::Volume;
    if (vol_mode) {
        std::set<size_t> object_ids;
        for (const auto& e : proj.selection.elements)
            object_ids.insert(e.object_id);
        for (size_t obj_id : object_ids)
            changes.append(update_instances_bed_placement(
                proj.project, proj.project.find_object_by_id(obj_id)->instances
            ));
    } else {
        changes = update_instances_bed_placement(proj.project, proj.selection.elements);
    }
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);
}

void SceneInteractor::invoke_slicing_input_changed(const Domain::BedRef& bed_instance) {
    invoke_listeners<ISlicingInputChangedListener>([&](auto listener) {
        listener->on_slicing_input_changed(bed_instance);
    });
}

void SceneInteractor::select_bed_instance_internal(const Domain::BedRef& bed_instance, bool force_update)
{
    if (!force_update && bed_instance == m_selected_bed_instance)
        return;

    Domain::Project::ConfigContainerList& ccs = m_projects.find(m_selected_project_id)->second.project.config_containers();
    for (auto& cc : ccs) {        
        Domain::ConfigContainer::BedInstanceList& instances = cc->bed_instances();
        for (auto& inst : instances) {
            inst->set_active(cc->id().id == bed_instance.config_container_id && inst->id().id == bed_instance.instance_id);
        }
    }

    m_selected_bed_instance = bed_instance;

    invoke_listeners<ISelectedBedInstanceChangedListener>([&](auto* l) {
        l->on_selected_bed_instance_changed(
            m_selected_project_id, bed_instance.config_container_id, bed_instance.instance_id);
    });
}

} // namespace Slic3r::Biz::Scene
