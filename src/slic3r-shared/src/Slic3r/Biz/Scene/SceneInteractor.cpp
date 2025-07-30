#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/ModelVolume.hpp"
#include "Slic3r/Biz/Scene/BedTracking.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <Slic3r/Assert.hpp>
#include <Slic3r/Log.hpp>
#include <fmt/ranges.h>
#include <vector>
#include <algorithm>

using Slic3r::Domain::BedContainer;
using Slic3r::Domain::SquareMatrix4d;
using Slic3r::Domain::Transform3d;
using Slic3r::Domain::Vec2d;
using Slic3r::Domain::Model;
using Slic3r::Domain::Project;
using Slic3r::Domain::ModelInstance;
using Slic3r::Domain::ConstModelInstanceList;
using Slic3r::Domain::ModelObject;

using namespace Slic3r::Biz;

namespace fmt {
template <>
struct formatter<Slic3r::Domain::ElementRef>
{
    // Parse format specifications (you can extend this if needed)
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    // Format the object
    template <typename FormatContext>
    auto format(const Slic3r::Domain::ElementRef& e, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(
            ctx.out(),
            "{{obj: {}, inst: {}, vol: {}}}",
            e.object_id,
            e.instance_id,
            e.volume_id
        );
    }
};
} // namespace fmt

namespace Slic3r::Biz::Scene {

static const Vec2d BED_GAP = {20.0, 20.0};
using Domain::Transformation;
using Domain::TriangleMesh;

namespace {

Transformation transform_product(const Transformation& orig_xform, const SceneInteractor::Transform& delta)
{
    Transform3d xform = orig_xform.get_matrix();
    xform             = delta * xform.matrix();
    return Transformation{xform};
}

Transformation transform_product(
    const SceneInteractor::Transform& orig_xform,
    const SceneInteractor::Transform& delta
)
{
    SceneInteractor::Transform xform = delta * orig_xform;
    return Transformation{Transform3d{xform}};
}

void transform_selection_instance_mode(
    const SceneInteractorProjectContext& proj,
    const SceneInteractor::Transform& relative_transform,
    TransformMemento& memento
)
{
    const bool initialize_memento = memento.elements.empty();
    const auto& sel               = proj.object_selection;
    DEBUG_ASSERT(sel.mode == SelectionMode::Instance);

    if (initialize_memento)
        memento.elements.reserve(sel.elements.size());
    for (const auto& e : sel.elements) {
        auto* inst = proj.project.find_instance_by_id(e.object_id, e.instance_id);
        if (initialize_memento)
            memento.elements.insert({e, {e, inst->get_matrix().matrix()}});
        inst->set_transformation(
            transform_product(memento.elements[e].original_xform, relative_transform)
        );
    }
}

void transform_selection_volume_mode(
    const SceneInteractorProjectContext& proj,
    const SceneInteractor::Transform& relative_transform,
    TransformMemento& memento
)
{
    const bool initialize_memento = memento.elements.empty();
    const auto& sel               = proj.object_selection;
    DEBUG_ASSERT(sel.mode == SelectionMode::Volume);

    if (initialize_memento)
        memento.elements.reserve(sel.elements.size());
    for (const auto& e : sel.elements) {
        DEBUG_ASSERT(e.volume_id != 0);
        auto* vol = proj.project.find_volume_by_id(e.object_id, e.volume_id);
        if (initialize_memento)
            memento.elements.insert({e, {e, vol->get_matrix().matrix()}});
        vol->set_transformation(
            transform_product(memento.elements[e].original_xform, relative_transform)
        );
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

void SceneInteractor::on_selected_config_container_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId container_id
)
{
    DEBUG_ASSERT(project_id == m_selected_project_id);
    m_selected_config_container_id = container_id;
}

const ObjectSelection& SceneInteractor::object_selection() const
{
    ASSERT(m_selected_project_id != Domain::INVALID_ID);
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());
    return it->second.object_selection;
}

void SceneInteractor::set_object_selection(const ObjectSelection& selection)
{
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());
    auto& project_context = it->second;
    ObjectSelection sel{.mode = selection.mode};
    for (const auto& e : selection.elements) {
        if (e.has_instance()) {
            sel.elements.push_back(e);
            continue;
        }
        const auto* obj = project_context.project.find_object_by_id(e.object_id);
        ASSERT(obj != nullptr);
        for (const auto& inst : obj->instances)
            sel.elements.push_back({e.object_id, inst->id().id});
    }

    DEBUG_ASSERT(sel.is_valid());
    project_context.object_selection = sel;
    invoke_listeners<ISceneSelectionChangedListener>([&](auto* l) {
        l->on_scene_selection_changed(m_selected_project_id, sel);
    });
}

void SceneInteractor::modify_selection(const std::function<void(ObjectSelection&)>& modifier)
{
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());
    auto& selection = it->second.object_selection;
    modifier(selection);
    DEBUG_ASSERT(selection.is_valid());
    invoke_listeners<ISceneSelectionChangedListener>([&](auto* l) {
        l->on_scene_selection_changed(m_selected_project_id, selection);
    });
}

const BedSelection& SceneInteractor::bed_selection() const {
    ASSERT(m_selected_project_id != Domain::INVALID_ID);
    const auto it{m_projects.find(m_selected_project_id)};
    ASSERT(it != m_projects.end());
    return it->second.bed_selection;
}

void SceneInteractor::new_object_from_mesh(TriangleMesh&& mesh)
{
    auto& project = m_workbench.project(m_selected_project_id);
    auto& obj     = *project.model().add_object();
    auto& vol     = *Algorithms::ModelObject::add_volume(&obj, std::move(mesh));
    auto& inst    = *obj.add_instance();
    const Domain::ElementRefs updated{{obj.id().id, inst.id().id}};
    // const Domain::ElementRefs updated_vols{{obj.id().id, inst.id().id, vol.id().id}};
    auto changes = update_instances_bed_placement(project, updated);

    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);
    invoke_listeners<ISceneChangedListener>([&](auto* l) {
        l->on_instance_added(m_selected_project_id, updated);
    });

    set_object_selection({SelectionMode::Instance, {updated}});
}

void SceneInteractor::add_volume_from_mesh(
    TriangleMesh&& mesh,
    Domain::ModelVolumeType volume_type,
    const Transform& xform
)
{
    auto& project        = m_workbench.project(m_selected_project_id);
    const ObjectSelection& sel = object_selection();
    DEBUG_ASSERT(sel.elements.size() == 1);
    size_t obj_id = sel.elements[0].object_id;
    Domain::ElementRefs updated;

    auto& obj = *project.find_object_by_id(obj_id);
    auto& vol = *Algorithms::ModelObject::add_volume(&obj, std::move(mesh), volume_type);
    vol.set_transformation(Transform3d{xform});
    updated.push_back({obj.id().id, obj.instances[0]->id().id, vol.id().id});

    invoke_listeners<ISceneChangedListener>([&](auto* l) {
        l->on_volume_added(m_selected_project_id, updated);
    });

    set_object_selection({SelectionMode::Volume, updated});
    update_selection_instance_bed_placement();
}

void SceneInteractor::add_instance(const Vec2d& offset)
{
    auto& project        = m_workbench.project(m_selected_project_id);
    const ObjectSelection& sel = object_selection();
    DEBUG_ASSERT(sel.elements.size() == 1);
    size_t obj_id = sel.elements[0].object_id;
    Domain::ElementRefs updated;

    auto& obj  = *project.find_object_by_id(obj_id);
    Transform3d trafo = obj.instances.back()->get_matrix();
    trafo.pretranslate(Domain::Vec3d(offset.x(), offset.y(), 0.));
    auto& inst = *obj.add_instance();
    inst.set_transformation(Transformation{trafo});
    updated.emplace_back(obj.id().id, inst.id().id);

    auto changes = update_instances_bed_placement(project, updated);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    invoke_listeners<ISceneChangedListener>([&](auto* l) {
        l->on_instance_added(m_selected_project_id, updated);
    });

    set_object_selection({SelectionMode::Instance, updated});
}

void SceneInteractor::notify_listener_on_objects(const Domain::ModelObjectPtrs& objects)
{
    auto& project = m_workbench.project(m_selected_project_id);
    for (const Domain::ModelObject* object : objects) {
        Domain::ElementRefs updated;
        SPDLOG_DEBUG("Notify listner obj {}", object->id().id);

        for (const Domain::ModelInstance* inst : object->instances)
            updated.emplace_back(object->id().id, inst->id().id, 0);

        auto changes = update_instances_bed_placement(project, updated);
        for (const auto& bed_ref : changes.updated_beds)
            invoke_slicing_input_changed(bed_ref);

        SPDLOG_DEBUG("- on_instance_added: {}", fmt::join(updated, ", "));
        invoke_listeners<ISceneChangedListener>([&](auto* l) {
            l->on_instance_added(m_selected_project_id, updated);
        });
    }
}

void SceneInteractor::notify_listener_on_objects()
{
    auto& project = m_workbench.project(m_selected_project_id);
    notify_listener_on_objects(project.model().objects);
}

void SceneInteractor::change_volume_meshes(RefMeshes&& meshes)
{
    Domain::Project& project = m_workbench.project(m_selected_project_id);
    Domain::ElementRefs removed_ids;
    Domain::ElementRefs updated_ids;
    removed_ids.reserve(meshes.size());
    updated_ids.reserve(meshes.size());
    std::vector<size_t> object_ids;
    object_ids.reserve(meshes.size());
    for (RefMesh& mesh : meshes) {
        const Domain::ElementRef& id        = mesh.first;
        Domain::TriangleMesh& triangle_mesh = mesh.second;
        Domain::ModelVolume* volume_ptr     = project.find_volume_by_id(id.object_id, id.volume_id);

        assert(volume_ptr != nullptr);
        if (volume_ptr == nullptr)
            return;

        Domain::ModelVolume& volume = *volume_ptr;
        volume.set_mesh(std::move(triangle_mesh));
        Algorithms::ModelVolume::calculate_convex_hull(volume);
        volume.set_new_unique_id();

        object_ids.push_back(id.object_id);
        removed_ids.emplace_back(id.object_id, 0, id.volume_id);
        updated_ids.emplace_back(id.object_id, 0, volume.id().id);
    }

    std::sort(object_ids.begin(), object_ids.end());
    object_ids.erase(std::unique(object_ids.begin(), object_ids.end()), object_ids.end());
    for (size_t object_id : object_ids) {
        Domain::ModelObject& object = *project.find_object_by_id(object_id);
        object.invalidate_bounding_box();
        Algorithms::ModelObject::ensure_on_bed(object, true); // disallow negative z
    }

    invoke_listeners<ISceneChangedListener>(
        [&removed_ids, &updated_ids, project_id = m_selected_project_id](auto* l) {
            l->on_volume_removed(project_id, removed_ids);
            l->on_volume_added(project_id, updated_ids);
        }
    );

    Domain::ElementRefs selection_ids;
    for (const auto& update_id : updated_ids) {
        Domain::ModelObject& object = *project.find_object_by_id(update_id.object_id);
        for (const auto& inst : object.instances)
            selection_ids.emplace_back(update_id.object_id, inst->id().id, update_id.volume_id);
    }
    set_object_selection({SelectionMode::Volume, selection_ids});
    auto changes = update_instances_bed_placement(project, selection_ids);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);
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
    Domain::ElementRefs updated;

    Domain::Project& project = m_workbench.project(m_selected_project_id);
    if (id.instance_id == 0) {
        auto obj       = project.find_object_by_id(id.object_id);
        obj->printable = is_printable;
        updated.reserve(obj->instances.size());
        for (auto& inst : obj->instances) {
            inst->printable = is_printable;
            updated.emplace_back(id.object_id, inst->id().id);
        }
    } else {
        project.find_instance_by_id(id.object_id, id.instance_id)->printable = is_printable;
        updated                                                              = {id};
    }

    auto changes = update_instances_bed_placement(project, updated);

    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);
}

void SceneInteractor::extract_selected_instances()
{
    const ObjectSelection& scene_selection = object_selection();
    if (scene_selection.empty() || scene_selection.mode != SelectionMode::Instance)
        return;

    bool all_instances_from_one_object = true;
    size_t object_id                   = scene_selection.elements[0].object_id;
    for (const auto& el : scene_selection.elements) {
        if (object_id != el.object_id) {
            all_instances_from_one_object = false;
            break;
        }
    }
    ASSERT(all_instances_from_one_object);

    Domain::Project& project = m_workbench.project(m_selected_project_id);
    Domain::Model& model     = project.model();

    ObjectSelection::ElementRefs to_remove = scene_selection.elements;
    Domain::ModelObjectPtrs new_objects;
    Domain::ModelObject* old_object = project.find_object_by_id(object_id);
    size_t sel_object_id            = old_object->id().id;

    if (old_object->instances.size() == to_remove.size()) {
        // split old_object instances into separate object

        for (int inst_cnt = int(old_object->instances.size()) - 1; inst_cnt > 0; inst_cnt--) {
            // make a copy of the active object
            Domain::ModelObject* new_object = model.add_object(*old_object);
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

        Domain::ElementRef stay_el({sel_object_id, old_object->instances[0]->id().id});
        to_remove.erase(
            std::remove_if(
                to_remove.begin(),
                to_remove.end(),
                [stay_el](const Domain::ElementRef& el) { return el == stay_el; }
            ),
            to_remove.end()
        );
    } else {
        // extract selected instances into separate object

        // make a copy of the active object
        Domain::ModelObject* new_object = model.add_object(*old_object);
        new_objects.emplace_back(new_object);

        // delete no needed instances from both objects
        for (size_t idx = old_object->instances.size() - 1; idx != size_t(-1); idx--) {
            if (scene_selection.is_selected({sel_object_id, old_object->instances[idx]->id().id}))
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

void SceneInteractor::prepare_loaded_project(Domain::Project& project)
{
    update_instances_bed_placement(project);
}

Domain::BedInstance& SceneInteractor::add_bed_instance(size_t config_container_id)
{
    SceneInteractorProjectContext& project_context{m_projects.find(m_selected_project_id)->second};
    auto& project               = project_context.project;
    Domain::ConfigContainer* cc = project.find_config_container(config_container_id);
    Domain::BedInstance& ret    = cc->add_bed_instance();
    ret.index                   = cc->bed_instances().size();

    m_bed_placement.layout(project, BED_GAP);

    // make copy
    Domain::ModelInstanceList unplaced = project.unplaced_model_instances();
    project.unplaced_model_instances().clear();
    auto changes = update_instances_bed_placement(project, unplaced, false);
    const Domain::BedRef updated{cc->id().id, ret.id().id};
    changes.updated_beds.insert(updated);

    if (project_context.bed_selection.empty()) {
        select_one_bed_instance(Domain::BedRef{cc->id().id, ret.id().id});
    }

    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    invoke_listeners<ISceneBedInstanceChangedListener>([&](auto* l) {
        l->on_bed_instance_added(m_selected_project_id, { updated });
    });
    return ret;
}

void SceneInteractor::remove_bed_instance(const Domain::BedRef& instance)
{
    auto& project               = m_projects.find(m_selected_project_id)->second.project;
    Domain::ConfigContainer* cc = project.find_config_container(instance.config_container_id);
    DEBUG_ASSERT(cc != nullptr);
    auto* bed_inst = Domain::find_by_id(cc->bed_instances(), instance.instance_id);

    auto insts = bed_inst->model_instances;

    cc->remove_bed_instance_by_id(instance.instance_id);

    // update bed_instance index
    size_t idx = 1;
    for (auto& inst : cc->bed_instances())
        inst->index = idx++;

    m_bed_placement.layout(project, BED_GAP);
    auto changes = update_instances_bed_placement(project, insts);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    invoke_listeners<ISlicingInputChangedListener>([&](auto* l) {
        l->on_slicing_input_removed(instance);
    });
    invoke_listeners<ISceneBedInstanceChangedListener>([&](auto* l) {
        l->on_bed_instance_removed(m_selected_project_id, { instance });
    });
}

void SceneInteractor::transform_bed_instance(const Domain::BedRef& instance, const Transform& xform)
{
    auto& proj                  = m_projects.find(m_selected_project_id)->second;
    Domain::ConfigContainer* cc = proj.project.find_config_container(instance.config_container_id);
    Domain::BedInstance& inst   = cc->find_bed_instance(instance.instance_id);
    inst.transformation         = Transformation{Transform3d{xform}};

    auto updated = inst.model_instances;
    inst.model_instances.clear();
    auto changes = update_instances_bed_placement(proj.project, updated, false);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    invoke_listeners<ISceneBedInstanceChangedListener>([&](auto* l) {
        l->on_bed_instance_transformed(m_selected_project_id, {instance}, TransformState::Completed);
    });
}

bool SceneInteractor::select_one_bed_instance(const Domain::BedRef& instance)
{
    SceneInteractorProjectContext& project_context{m_projects.find(m_selected_project_id)->second};
    if (!project_context.bed_selection.select_one(instance)) {
        return false;
    }

    invoke_listeners<ISelectedBedInstancesChangedListener>([&](auto* l) {
        l->on_selected_bed_instances_changed(
            m_selected_project_id,
            project_context.bed_selection
        );
    });
    return true;
}

bool SceneInteractor::toggle_bed_instance(const Domain::BedRef& instance)
{
    SceneInteractorProjectContext& project_context{m_projects.find(m_selected_project_id)->second};
    if (!project_context.bed_selection.toggle(instance)) {
        return false;
    }

    invoke_listeners<ISelectedBedInstancesChangedListener>([&](auto* l) {
        l->on_selected_bed_instances_changed(
            m_selected_project_id,
            project_context.bed_selection
        );
    });
    return true;
}

const Domain::Project::ConfigContainerList& SceneInteractor::selected_project_config_containers() const
{
    return m_projects.find(m_selected_project_id)->second.project.config_containers();
}

const Domain::ModelInstanceList& SceneInteractor::selected_project_unplaced_model_instances() const
{
    return m_projects.find(m_selected_project_id)->second.project.unplaced_model_instances();
}

const BedContainer::BedList& SceneInteractor::selected_project_beds() const
{
    const Project& project{m_projects.find(m_selected_project_id)->second.project};
    return project.bed_container().beds();
}

const ConstModelInstanceList SceneInteractor::selected_project_instances() const
{
    const Project& project{m_projects.find(m_selected_project_id)->second.project};
    const Model& model{project.model()};

    ConstModelInstanceList result;
    for (const ModelObject* object : model.objects) {
        for (const ModelInstance* instance : object->instances) {
            result.push_back(instance);
        }
    }
    return result;
}

void SceneInteractor::transform_selection(const Transform& relative_transform)
{
    TransformMemento memento;
    transform_selection(relative_transform, memento);
    finalize_transform_selection(memento, false);
}

void SceneInteractor::transform_selection(const SquareMatrix4d& relative_transform, TransformMemento& memento)
{
    auto& proj               = m_projects.find(m_selected_project_id)->second;
    const bool instance_mode = proj.object_selection.mode == SelectionMode::Instance;
    if (instance_mode)
        transform_selection_instance_mode(proj, relative_transform, memento);
    else
        transform_selection_volume_mode(proj, relative_transform, memento);
    update_selection_instance_bed_placement();
    invoke_listeners<ISceneChangedListener>([&](ISceneChangedListener* l) {
        if (instance_mode)
            l->on_instance_transformed(
                m_selected_project_id,
                proj.object_selection.elements,
                TransformState::InProgress
            );
        else
            l->on_volume_transformed(
                m_selected_project_id,
                proj.object_selection.elements,
                TransformState::InProgress
            );
    });
    invoke_listeners<ISceneSelectionChangedListener>([&](ISceneSelectionChangedListener* l) {
        l->on_scene_selection_transformed(m_selected_project_id, proj.object_selection);
    });
}

void SceneInteractor::transform_instances(const InstanceTransformations& transformations)
{
    Project& project{m_projects.find(m_selected_project_id)->second.project};

    std::vector<Domain::ElementRef> elements;
    for (const auto& [element, trafo] : transformations) {
        ModelInstance* instance{project.find_instance_by_id(element.object_id, element.instance_id)};
        const SquareMatrix4d initial_trafo{instance->get_transformation().get_matrix().matrix()};
        const SquareMatrix4d new_trafo{trafo * initial_trafo};
        instance->set_transformation(Transformation{Transform3d{new_trafo}});
        elements.push_back(element);
    }

    const BedTrackingChanges changes{update_instances_bed_placement(project, elements)};
    for (const auto& bed_ref : changes.updated_beds) {
        invoke_slicing_input_changed(bed_ref);
    }

    invoke_listeners<ISceneChangedListener>([&](ISceneChangedListener* l) {
        l->on_instance_transformed(m_selected_project_id, elements, TransformState::Completed);
    });
}

void SceneInteractor::finalize_transform_selection(TransformMemento& memento, bool canceled)
{
    auto& proj          = m_projects.find(m_selected_project_id)->second;
    const bool vol_mode = proj.object_selection.mode == SelectionMode::Volume;

    if (canceled) {
        for (const auto& [_, e] : memento.elements) {
            const Transformation xform{Transform3d{e.original_xform}};
            if (vol_mode) {
                auto* vol = proj.project.find_volume_by_id(e.element.object_id, e.element.volume_id);
                vol->set_transformation(xform);
            } else {
                auto* inst = proj.project
                                 .find_instance_by_id(e.element.object_id, e.element.instance_id);
                inst->set_transformation(xform);
            }
        }

        update_selection_instance_bed_placement();
    }

    invoke_listeners<ISceneChangedListener>([&](ISceneChangedListener* l) {
        if (vol_mode)
            l->on_volume_transformed(
                m_selected_project_id,
                proj.object_selection.elements,
                canceled ? TransformState::Canceled : TransformState::Completed
            );
        else
            l->on_instance_transformed(
                m_selected_project_id,
                proj.object_selection.elements,
                canceled ? TransformState::Canceled : TransformState::Completed
            );
    });
    memento.reset();
}

void SceneInteractor::update_selection_instance_bed_placement()
{
    BedTrackingChanges changes;
    auto& proj          = m_projects.find(m_selected_project_id)->second;
    const bool vol_mode = proj.object_selection.mode == SelectionMode::Volume;
    if (vol_mode) {
        std::set<size_t> object_ids;
        for (const auto& e : proj.object_selection.elements)
            object_ids.insert(e.object_id);
        for (size_t obj_id : object_ids)
            changes.append(update_instances_bed_placement(
                proj.project,
                proj.project.find_object_by_id(obj_id)->instances
            ));
    } else {
        changes = update_instances_bed_placement(proj.project, proj.object_selection.elements);
    }
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);
}

void SceneInteractor::invoke_slicing_input_changed(const Domain::BedRef& bed_instance)
{
    invoke_listeners<ISlicingInputChangedListener>([&](auto listener) {
        listener->on_slicing_input_changed(bed_instance);
    });
}

} // namespace Slic3r::Biz::Scene
