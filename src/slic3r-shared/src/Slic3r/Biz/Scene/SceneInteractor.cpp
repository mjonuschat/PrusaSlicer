#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Types.hpp"

#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/ModelVolume.hpp"
#include "Slic3r/Biz/Scene/BedFactory.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <Slic3r/Assert.hpp>
#include <Slic3r/Log.hpp>
#include <Slic3r/Directories.hpp>
#include <fmt/ranges.h>
#include <vector>
#include <algorithm>
#include <boost/filesystem/path.hpp>

using Slic3r::Domain::BedContainer;
using Slic3r::Domain::ConstModelInstanceList;
using Slic3r::Domain::Model;
using Slic3r::Domain::ModelInstance;
using Slic3r::Domain::ModelObject;
using Slic3r::Domain::Project;
using Slic3r::Domain::SquareMatrix4d;
using Slic3r::Domain::Transform3d;
using Slic3r::Domain::Vec2d;
using Slic3r::Domain::Vec3d;

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
        return fmt::format_to(ctx.out(), "{{obj: {}, inst: {}, vol: {}}}", e.object_id, e.instance_id, e.volume_id);
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

Transformation transform_product(const SceneInteractor::Transform& orig_xform, const SceneInteractor::Transform& delta)
{
    DEBUG_ASSERT(fabs(delta.determinant()) > 1e-9);
    DEBUG_ASSERT(fabs(orig_xform.determinant()) > 1e-9);

    SceneInteractor::Transform xform = delta * orig_xform;

    DEBUG_ASSERT(fabs(xform.determinant()) > 1e-9);

    return Transformation{Transform3d{xform}};
}

/**
 * @brief Determines if a transform must be applied to a volume node.
 * * This is required if the transform contains any rotation component around
 * the X or Y axes, which would change the direction of the Z-axis.
 * Scaling along the Z-axis is permitted for instance-level transforms.
 *
 * @param xform The relative transformation matrix to check.
 * @return True if the transform has X/Y rotation or a delta z different from zero, false otherwise.
 */
bool requires_volume_transform(const Eigen::Matrix4d& xform)
{
    // Extract the third column of the linear part (rotation/scaling).
    // This vector shows where the original Z-axis (0,0,1) points after transformation.
    Eigen::Vector3d transformed_z = xform.topLeftCorner<3, 3>().col(2);

    // Handle the edge case where the Z-axis is scaled to zero length.
    // Such a transform is highly distorting and should be flagged.
    if (transformed_z.norm() < 1e-9) {
        return true;
    }

    // Normalize the vector to get its direction, ignoring any scaling.
    transformed_z.normalize();

    double dz = xform(2, 3);

    // Check if the direction is still parallel to the original Z-axis and if there is a delta z.
    // We use isApprox() for safe floating-point comparison. A dot product
    // check like `abs(transformed_z.dot(Eigen::Vector3d::UnitZ()))` could also work.
    return !transformed_z.isApprox(Eigen::Vector3d::UnitZ()) || std::abs(dz) > Domain::EPSILON;
}

SelectionMode
transform_selection_instance_mode(const SceneInteractorProjectContext& proj, const SceneInteractor::Transform& relative_transform, TransformMemento& memento)
{
    const bool initialize_memento = memento.elements.empty();
    const auto& sel               = proj.object_selection;
    DEBUG_ASSERT(sel.mode == SelectionMode::Instance);

    const bool volume_transform_mode = memento.forced_volume_mode || requires_volume_transform(relative_transform);

    if (initialize_memento)
        memento.elements.reserve(sel.elements.size());

    std::set<size_t> object_ids;

    for (const auto& e : sel.elements) {
        auto* inst = proj.project.find_instance_by_id(e.object_id, e.instance_id);
        if (volume_transform_mode)
            object_ids.insert(inst->get_object()->id().id);
        else {
            if (initialize_memento)
                memento.elements.insert({e, {e, inst->get_matrix().matrix()}});
            inst->set_transformation(transform_product(memento.elements[e].original_xform, relative_transform));
        }
    }

    if (!volume_transform_mode)
        return SelectionMode::Instance;

    memento.forced_volume_mode = true;

    for (size_t object_id : object_ids) {
        auto* obj = proj.project.find_object_by_id(object_id);

        auto first_inst_it = std::ranges::find_if(
            proj.object_selection.elements,
            [object_id](const auto& e) { return e.object_id == object_id; }
        );
        ASSERT(first_inst_it != proj.object_selection.elements.end());
        auto* first_inst = proj.project.find_instance_by_id(first_inst_it->object_id, first_inst_it->instance_id);
        const auto parent = first_inst->get_matrix();

        // We need to take the `relative_transform` and turn it into local one
        const SceneInteractor::Transform
            volume_relative_transform = (parent.inverse() * relative_transform * parent).matrix();
        for (auto* vol : obj->volumes) {
            Domain::ElementRef e{object_id, first_inst->id().id, vol->id().id};
            if (!memento.elements.contains(e))
                memento.elements.insert({e, {e, vol->get_matrix().matrix()}});
            vol->set_transformation(transform_product(memento.elements[e].original_xform, volume_relative_transform));
        }
    }
    return SelectionMode::Volume;
}

void transform_selection_volume_mode(const SceneInteractorProjectContext& proj, const SceneInteractor::Transform& relative_transform, TransformMemento& memento)
{
    const bool initialize_memento = memento.elements.empty();
    const auto& sel               = proj.object_selection;
    DEBUG_ASSERT(sel.mode == SelectionMode::Volume);
    ASSERT(!sel.elements.empty());
    const auto& first_el = sel.elements[0];
    // assert that all elements are of same instance
    DEBUG_ASSERT(
        std::all_of(
            ++sel.elements.begin(),
            sel.elements.end(),
            [inst_id = first_el.instance_id](const auto& el) { return el.instance_id == inst_id; }
        )
    );
    const auto* first_inst = proj.project.find_instance_by_id(first_el.object_id, first_el.instance_id);
    const auto parent = first_inst->get_matrix();
    // We need to take the `relative_transform` which is in world space and turn it into local space
    const SceneInteractor::Transform
        volume_relative_transform = (parent.inverse() * relative_transform * parent).matrix();
    if (initialize_memento)
        memento.elements.reserve(sel.elements.size());
    for (const auto& e : sel.elements) {
        DEBUG_ASSERT(e.volume_id != 0);
        auto* vol = proj.project.find_volume_by_id(e.object_id, e.volume_id);
        if (initialize_memento)
            memento.elements.insert({e, {e, vol->get_matrix().matrix()}});
        vol->set_transformation(transform_product(memento.elements[e].original_xform, volume_relative_transform));
    }
}

} // namespace

SceneInteractor::SceneInteractor(Domain::Workbench& workbench) : m_workbench(workbench) {}

void SceneInteractor::on_selected_project_changed(size_t index)
{
    BedSelection selection{};
    selection.on_change = [this, index](const BedSelection& bed_selection)
    {
        invoke_listeners<ISelectedBedInstancesChangedListener>(
            [&](auto* l) { l->on_selected_bed_instances_changed(index, bed_selection); }
        );
    };

    auto& project = m_workbench.project(index);
    if (m_projects.count(index) == 0)
        m_projects.emplace(index, SceneInteractorProjectContext{project, selection});
    m_selected_project_id = index;
}

void SceneInteractor::on_selected_config_container_changed(Domain::SelectionId project_id, Domain::SelectionId container_id)
{
    DEBUG_ASSERT(project_id == m_selected_project_id);
    if (m_selected_config_container_id == container_id)
        return;

    m_selected_config_container_id = container_id;
    const auto& cc = m_workbench.project(project_id).find_config_container(container_id);
    ASSERT(cc->bed_instances().size() >= 1);
    const auto& bed_instances = cc->bed_instances();
    auto& selection           = bed_selection();
    if (selection.empty() || selection.last_selected_bed().config_container_id != container_id) {
        // Select CC's first bed if not already selected any of CC's
        selection.select_one({container_id, bed_instances.front()->id().id});
    }
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
            sel.elements.emplace_back(e.object_id, inst->id().id);
    }

    DEBUG_ASSERT(sel.is_valid());
    project_context.object_selection = sel;
    invoke_listeners<ISceneSelectionChangedListener>(
        [&](auto* l) { l->on_scene_selection_changed(m_selected_project_id, sel); }
    );
}

void SceneInteractor::modify_selection(const std::function<void(ObjectSelection&)>& modifier)
{
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());
    auto& selection = it->second.object_selection;
    modifier(selection);
    DEBUG_ASSERT(selection.is_valid());
    invoke_listeners<ISceneSelectionChangedListener>(
        [&](auto* l) { l->on_scene_selection_changed(m_selected_project_id, selection); }
    );
}

const BedSelection& SceneInteractor::bed_selection() const
{
    const BedSelection* result{bed_selection(m_selected_project_id)};
    return *ASSERT_VAL(result);
}

BedSelection& SceneInteractor::bed_selection()
{
    BedSelection* result{bed_selection(m_selected_project_id)};
    return *ASSERT_VAL(result);
}

const BedSelection* SceneInteractor::bed_selection(const Domain::SelectionId project_id) const
{
    ASSERT(project_id != Domain::INVALID_ID);
    const auto it{m_projects.find(project_id)};
    if (it == m_projects.end()) {
        return nullptr;
    }
    return &it->second.bed_selection;
}

BedSelection* SceneInteractor::bed_selection(const Domain::SelectionId project_id)
{
    return const_cast<BedSelection*>(static_cast<const SceneInteractor*>(this)->bed_selection(project_id));
}

void SceneInteractor::new_object_from_mesh(Domain::TriangleMesh&& mesh, const std::string& name)
{
    auto& project = m_workbench.project(m_selected_project_id);
    auto& obj     = *project.model().add_object();
    auto& vol     = *Algorithms::ModelObject::add_volume(&obj, std::move(mesh));
    auto& inst    = *obj.add_instance();
    const Domain::ElementRefs updated{{obj.id().id, inst.id().id}};
    // const Domain::ElementRefs updated_vols{{obj.id().id, inst.id().id, vol.id().id}};
    auto changes = m_bed_tracking.update_instances_bed_placement(project, updated);

    obj.name      = vol.name = name;

    for (const auto& bed_ref : changes.updated_beds) {
        invoke_slicing_input_changed(bed_ref);
    }
    invoke_listeners<ISceneChangedListener>(
        [&](auto* l) { l->on_instance_added(m_selected_project_id, updated); }
    );

    set_object_selection({SelectionMode::Instance, {updated}});
}

void SceneInteractor::add_new_objects(const std::vector<Domain::ModelObject*>& objects)
{
    auto& project = m_workbench.project(m_selected_project_id);

    Domain::ModelObjectPtrs new_objects;
    for (Domain::ModelObject* object : objects) {
        new_objects.emplace_back(project.model().add_object(*object));
    }

    notify_listener_on_objects(new_objects);
}

void SceneInteractor::add_volume_from_mesh(TriangleMesh&& mesh, Domain::ModelVolumeType volume_type, const std::string& name, const Transform& xform)
{
    auto& project              = m_workbench.project(m_selected_project_id);
    const ObjectSelection& sel = object_selection();
    ASSERT(sel.mode == SelectionMode::Volume || sel.only_single_object());
    const size_t obj_id  = sel.elements[0].object_id;
    const size_t inst_id = sel.elements[0].instance_id;
    Domain::ElementRefs updated;

    auto& obj = *project.find_object_by_id(obj_id);
    auto& vol = *Algorithms::ModelObject::add_volume(&obj, std::move(mesh), volume_type);
    vol.name  = name;
    if (xform != Domain::SquareMatrix4d::Identity()) {
        // Apply transformations only if explicitly set.
        // By default, the object controls the transformation of the volume added to it.
        vol.set_transformation(Transform3d{xform});
    }
    updated.emplace_back(obj.id().id, inst_id, vol.id().id);

    invoke_listeners<ISceneChangedListener>(
        [&](auto* l) { l->on_volume_added(m_selected_project_id, updated); }
    );

    set_object_selection({SelectionMode::Volume, updated});
    update_selection_instance_bed_placement();
}

void SceneInteractor::add_instance(const Vec2d& offset)
{
    auto& project              = m_workbench.project(m_selected_project_id);
    const ObjectSelection& sel = object_selection();
    DEBUG_ASSERT(sel.elements.size() >= 1);
    size_t obj_id = sel.elements[0].object_id;
    Domain::ElementRefs updated;

    auto& obj         = *project.find_object_by_id(obj_id);
    Transform3d trafo = obj.instances.back()->get_matrix();
    trafo.pretranslate(Domain::Vec3d(offset.x(), offset.y(), 0.));
    auto& inst = *obj.add_instance();
    inst.set_transformation(Transformation{trafo});
    updated.emplace_back(obj.id().id, inst.id().id);

    auto changes = m_bed_tracking.update_instances_bed_placement(project, updated);
    for (const auto& bed_ref : changes.updated_beds) {
        invoke_slicing_input_changed(bed_ref);
    }

    invoke_listeners<ISceneChangedListener>(
        [&](auto* l) { l->on_instance_added(m_selected_project_id, updated); }
    );

    set_object_selection({SelectionMode::Instance, updated});
}

void SceneInteractor::notify_listener_on_objects(const Domain::ModelObjectPtrs& objects)
{
    auto& project = m_workbench.project(m_selected_project_id);
    Domain::ElementRefs updated;
    for (const Domain::ModelObject* object : objects) {
        SPDLOG_DEBUG("Notify listener obj {}", object->id().id);

        for (const Domain::ModelInstance* inst : object->instances)
            updated.emplace_back(object->id().id, inst->id().id, 0);

        auto changes = m_bed_tracking.update_instances_bed_placement(project, updated);
        for (const auto& bed_ref : changes.updated_beds)
            invoke_slicing_input_changed(bed_ref);

    }
    if (updated.size()) {
        SPDLOG_DEBUG("- on_instance_added: {}", fmt::join(updated, ", "));
        invoke_listeners<ISceneChangedListener>(
            [&](auto* l) { l->on_instance_added(m_selected_project_id, updated); }
        );
    }
    set_object_selection({SelectionMode::Instance, {updated}});
}

void SceneInteractor::notify_listener_on_objects(const Domain::Project& project)
{
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
        [&removed_ids, &updated_ids, project_id = m_selected_project_id](auto* l)
        {
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
    auto changes = m_bed_tracking.update_instances_bed_placement(project, selection_ids);
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

    auto changes = m_bed_tracking.update_instances_bed_placement(project, updated);

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

    // notify listener on changes

    notify_listener_on_objects(new_objects);
    invoke_listeners<ISceneChangedListener>(
        [&](auto* l) { l->on_instance_removed(m_selected_project_id, to_remove); }
    );
}

std::optional<std::string> SceneInteractor::delete_selected_elements()
{
    Domain::Project& project               = m_workbench.project(m_selected_project_id);
    Domain::Model& model                   = project.model();
    const ObjectSelection& scene_selection = object_selection();
    ObjectSelection::ElementRefs to_remove = scene_selection.elements;

    std::optional<std::string> last_solid_part_name;
    ObjectSelection new_selection = {};

    BedTrackingChanges changes;

    // Remove selected elements from the model

    if (scene_selection.mode == SelectionMode::Instance) {
        // Delete selected instances from its objects
        for (const auto& el : to_remove) {
            Domain::ModelObject* object = project.find_object_by_id(el.object_id);
            for (size_t idx = object->instances.size() - 1; idx != size_t(-1); idx--) {
                if (object->instances[idx]->id().id == el.instance_id) {
                    remove_instance_from_bed(project, object->instances[idx], changes);
                    object->delete_instance(idx);
                    break;
                }
            }
        }

        // Identify objects with no associated instances.
        // Delete such objects if any are found.
        for (size_t idx = model.objects.size() - 1; idx != size_t(-1); idx--) {
            if (model.objects[idx]->instances.size() == 0) {
                model.delete_object(idx);
            }
        }

        // There is nothing to select

    } else if (scene_selection.mode == SelectionMode::Volume) {
        // Delete selected volumes from the object.
        Domain::ModelObject* object = project.find_object_by_id(to_remove.front().object_id);

        // We must track the number of "solid part" volumes in the object
        // and prevent deletion of the last remaining one.
        int solid_cnt = 0;
        for (auto vol : object->volumes)
            if (vol->is_model_part())
                ++solid_cnt;

        Domain::ElementRef last_solid_part_el;
        for (const auto& el : to_remove) {
            for (size_t idx = object->volumes.size() - 1; idx != size_t(-1); idx--) {
                const Domain::ModelVolume* volume = object->volumes[idx];
                if (volume->id().id == el.volume_id) {
                    if (volume->is_model_part()) {
                        if (solid_cnt == 1) {
                            // If the user attempts to delete the last solid part,
                            // store its name and related element in selection.
                            // And display an error dialog afterward.
                            last_solid_part_name = volume->name;
                            last_solid_part_el   = el;
                            continue;
                        }
                        --solid_cnt;
                    }
                    object->delete_volume(idx);
                    break;
                }
            }
        }

        if (last_solid_part_name && last_solid_part_name.has_value()) {
            // Don't remove last_solid_part_el from the scene graph
            to_remove.erase(
                std::remove_if(
                    to_remove.begin(),
                    to_remove.end(),
                    [last_solid_part_el](const Domain::ElementRef& el)
                    { return el == last_solid_part_el; }
                ),
                to_remove.end()
            );
        }

        // Select the object from which volumes were deleted.
        new_selection.mode = SelectionMode::Instance;
        for (const Domain::ModelInstance* instance : object->instances) {
            new_selection.elements.emplace_back(Domain::ElementRef(object->id().id, instance->id().id));
        }

        changes = m_bed_tracking.update_instances_bed_placement(project, to_remove, scene_selection.mode == SelectionMode::Instance);
    }

    // Notify listeners on changes

    invoke_listeners<ISceneChangedListener>(
        [&](auto* l)
        {
            if (scene_selection.mode == SelectionMode::Instance) {
                l->on_instance_removed(m_selected_project_id, to_remove);
            } else if (scene_selection.mode == SelectionMode::Volume) {
                l->on_volume_removed(m_selected_project_id, to_remove);
            }
        }
    );

    set_object_selection(new_selection);

    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    return last_solid_part_name;
}

void SceneInteractor::prepare_added_project(Domain::Project& project)
{
    m_bed_tracking.update_instances_bed_placement(project);
    for (auto& cc : project.config_containers()) {
        size_t index = 0;
        for (auto& bed_instance : cc->bed_instances()) {
            bed_instance->set_index(++index);
        }
    }

    notify_listener_on_objects(project);
    layout_after_project_load(project);

}

Domain::BedInstance& SceneInteractor::add_bed_instance(size_t config_container_id)
{
    SceneInteractorProjectContext& project_context{m_projects.find(m_selected_project_id)->second};
    auto& project               = project_context.project;
    Domain::ConfigContainer* cc = project.find_config_container(config_container_id);
    Domain::BedInstance& ret    = cc->add_bed_instance();
    ret.set_index(cc->bed_instances().size());

    auto changed_elements = m_bed_placement.layout(project, BED_GAP);

    // make copy
    Domain::ModelInstanceList unplaced = project.unplaced_model_instances();
    project.unplaced_model_instances().clear();
    auto changes = m_bed_tracking.update_instances_bed_placement(project, unplaced, false);
    const Domain::BedRef updated{cc->id().id, ret.id().id};
    changes.updated_beds.insert(updated);

    if (project_context.bed_selection.empty()) {
        project_context.bed_selection.select_one(Domain::BedRef{cc->id().id, ret.id().id});
    }

    invoke_listeners<ISceneChangedListener>(
        [&](auto* l)
        {
            l->on_instance_transformed(
                m_selected_project_id,
                changed_elements,
                TransformState::Completed,
                changes
            );
        }
    );

    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    invoke_listeners<ISceneBedInstanceChangedListener>(
        [&](auto* l) { l->on_bed_instance_updated(m_selected_project_id, {updated}); }
    );
    return ret;
}

void SceneInteractor::layout_after_project_load(Domain::Project& added_project)
{
    ASSERT(std::all_of(added_project.config_containers().begin(), added_project.config_containers().end(),
           [](const std::unique_ptr<Domain::ConfigContainer>& cc){ return ! cc->bed_instances().empty(); }));
    m_bed_tracking.update_instances_bed_placement(added_project);
    auto updated = m_bed_placement.layout(added_project, BED_GAP);

    bed_selection().select_one(Domain::BedRef({added_project.config_containers().front()->id().id, added_project.config_containers().front()->bed_instances().front()->id().id}));
    auto changes = m_bed_tracking.update_instances_bed_placement(added_project);

    Domain::BedRefs bed_refs;
    for (auto& cc : added_project.config_containers()) {
        for (auto& bed_instance : cc->bed_instances())
            bed_refs.push_back({cc->id().id, bed_instance->id().id});
    }

    invoke_listeners<ISceneChangedListener>(
        [&](auto* l)
        {
            l->on_instance_transformed(
                m_selected_project_id,
                updated,
                TransformState::Completed,
                changes
            );
        }
    );

    invoke_listeners<ISceneBedInstanceChangedListener>(
        [&](auto* l) { l->on_bed_instance_updated(m_selected_project_id, bed_refs); }
    );
}

void SceneInteractor::remove_bed_instance(const Domain::BedRef& instance)
{
    auto& project               = m_projects.find(m_selected_project_id)->second.project;
    Domain::ConfigContainer* cc = project.find_config_container(instance.config_container_id);
    ASSERT(cc != nullptr);
    auto* bed_inst = Domain::find_by_id(cc->bed_instances(), instance.instance_id);

    auto insts = bed_inst->model_instances;

    auto& selection = bed_selection();
    selection.remove(instance);
    cc->remove_bed_instance_by_id(instance.instance_id);

    // update bed_instance index
    size_t idx = 1;
    Domain::BedRefs updated;
    for (auto& inst : cc->bed_instances()) {
        auto index = idx++;
        if (index != inst->index()) {
            inst->set_index(index);
            updated.emplace_back(cc->id().id, inst->id().id);
        }
    }
    auto updated_instances = m_bed_placement.layout(project, BED_GAP);

    if (bed_selection().empty()) {
        // ensure one bed instance is selected
        ASSERT(!cc->bed_instances().empty());
        selection.select_one({ cc->id().id, cc->bed_instances().front()->id().id });
    }

    invoke_listeners<ISceneBedInstanceChangedListener>(
        [&](auto* l) { l->on_bed_instance_updated(m_selected_project_id, updated); }
    );

    auto changes = m_bed_tracking.update_instances_bed_placement(project, insts);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    invoke_listeners<ISceneChangedListener>(
        [&](auto* l)
        {
            l->on_instance_transformed(
                m_selected_project_id,
                updated_instances,
                TransformState::Completed,
                changes
            );
        }
    );

    invoke_listeners<ISlicingInputChangedListener>(
        [&](auto* l) { l->on_slicing_input_removed(instance); }
    );
    invoke_listeners<ISceneBedInstanceChangedListener>(
        [&](auto* l) { l->on_bed_instance_removed(m_selected_project_id, {instance}); }
    );

    if (cc->bed_instances().empty()) {
        add_bed_instance(cc->id().id);
    }
}

void SceneInteractor::transform_bed_instance(const Domain::BedRef& instance, const Transform& xform)
{
    auto& proj                  = m_projects.find(m_selected_project_id)->second;
    Domain::ConfigContainer* cc = proj.project.find_config_container(instance.config_container_id);
    Domain::BedInstance& inst   = cc->find_bed_instance(instance.instance_id);
    inst.transformation         = Transformation{Transform3d{xform}};

    auto updated = inst.model_instances;
    inst.model_instances.clear();
    auto changes = m_bed_tracking.update_instances_bed_placement(proj.project, updated, false);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    invoke_listeners<ISceneBedInstanceChangedListener>(
        [&](auto* l)
        {
            l->on_bed_instance_transformed(m_selected_project_id, {instance}, TransformState::Completed);
        }
    );
}

void SceneInteractor::update_config_container_bed(Domain::Project& project, const Domain::SelectionId& config_container_id)
{
    Domain::ConfigContainer* config_container{project.find_config_container(config_container_id)};
    if (config_container == nullptr) {
        return;
    }
    const auto& selected_printer_preset{config_container->selected_preset()};
    Domain::Bed& bed{get_or_create_bed(project.bed_container(), selected_printer_preset, Slic3r::resources_dir())};

    const Domain::Bed* previous_bed{&config_container->bed()};
    if (previous_bed != &bed) {
        config_container->set_bed(bed);

        Domain::BedRefs bed_refs;
        Domain::ElementRefs changed_instances;
        for (auto& bed_instance : config_container->bed_instances()) {
            bed_instance->bed = bed;
            bed_refs.push_back({config_container->id().id, bed_instance->id().id});
            for (const auto* mi : bed_instance->model_instances) {
                changed_instances.emplace_back(mi->get_object()->id().id, mi->id().id);
            }
        }
        auto updated = m_bed_placement.layout(project, BED_GAP);

        const auto it{std::ranges::find_if(
            project.config_containers(),
            [&](const auto& config_container) { return &config_container->bed() == previous_bed; }
        )};

        if (it == project.config_containers().end()) {
            project.bed_container().remove(previous_bed);
        }

        auto changes = m_bed_tracking.update_instances_bed_placement(project, changed_instances);


        invoke_listeners<ISceneChangedListener>(
            [&](auto* l)
            {
                l->on_instance_transformed(
                    m_selected_project_id,
                    updated,
                    TransformState::Completed,
                    changes
                );
            }
        );

        invoke_listeners<ISceneBedInstanceChangedListener>(
            [&](auto* l) { l->on_bed_instance_updated(m_selected_project_id, bed_refs); }
        );
    }
}

void SceneInteractor::on_preset_selection_changed(Domain::SelectionId project_id, Domain::SelectionId config_container_id, Preset::PresetItemType type)
{
    if (type != Preset::PresetItemType::PrinterPreset) {
        return;
    }

    Domain::Project& project{m_workbench.project(project_id)};
    update_config_container_bed(project, config_container_id);
}

void SceneInteractor::on_preset_value_changed(Domain::SelectionId project_id, Domain::SelectionId config_container_id, const Domain::ConfigItem& item)
{
    const std::vector<std::string> bed_related_keys{
        "bed_shape",
        "max_print_height",
        "bed_custom_model",
        "bed_custom_texture",
    };

    if (std::ranges::find(bed_related_keys, item.def().name) == bed_related_keys.end()) {
        return;
    }

    Domain::Project& project{m_workbench.project(project_id)};
    update_config_container_bed(project, config_container_id);
}

const Domain::Project::ConfigContainerList& SceneInteractor::selected_project_config_containers() const
{
    return m_projects.find(m_selected_project_id)->second.project.config_containers();
}

const Domain::ModelInstanceList& SceneInteractor::unplaced_model_instances(const Domain::SelectionId project_id) const
{
    return m_projects.find(project_id)->second.project.unplaced_model_instances();
}

const Domain::ModelInstanceList& SceneInteractor::selected_project_unplaced_model_instances() const
{
    return unplaced_model_instances(m_selected_project_id);
}

const BedContainer::BedList& SceneInteractor::selected_project_beds() const
{
    const Project& project{m_projects.find(m_selected_project_id)->second.project};
    return project.bed_container().beds();
}

void SceneInteractor::transform_selection(const Transform& relative_transform)
{
    TransformMemento memento;
    transform_selection(relative_transform, memento);
    finalize_transform_selection(memento, false);
}

void SceneInteractor::transform_selection(const SquareMatrix4d& relative_transform, TransformMemento& memento)
{
    DEBUG_ASSERT(fabs(relative_transform.determinant()) > 1e-9);
    auto& proj         = m_projects.find(m_selected_project_id)->second;
    bool instance_mode = proj.object_selection.mode == SelectionMode::Instance;
    auto selection     = proj.object_selection;
    if (instance_mode) {
        auto final_mode = transform_selection_instance_mode(proj, relative_transform, memento);
        if (final_mode == SelectionMode::Volume) {
            instance_mode  = false;
            selection.mode = SelectionMode::Volume;
            selection.elements.clear();
            for (const auto& e : memento.elements)
                selection.elements.push_back(e.first);
        }
    } else
        transform_selection_volume_mode(proj, relative_transform, memento);
    BedTrackingChanges changes = update_selection_instance_bed_placement(memento.forced_volume_mode);
    invoke_listeners<ISceneChangedListener>(
        [&](ISceneChangedListener* l)
        {
            if (instance_mode)
                l->on_instance_transformed(m_selected_project_id, selection.elements, TransformState::InProgress, changes);
            else
                l->on_volume_transformed(m_selected_project_id, selection.elements, TransformState::InProgress, changes);
        }
    );
    invoke_listeners<ISceneSelectionChangedListener>(
        [&](ISceneSelectionChangedListener* l)
        { l->on_scene_selection_transformed(m_selected_project_id, selection); }
    );
}

void SceneInteractor::transform_instances(const InstanceTransforms& transformations)
{
    Project& project{m_projects.find(m_selected_project_id)->second.project};

    std::vector<Domain::ElementRef> elements;
    for (const InstanceTransform2D& trafo : transformations) {
        ModelInstance* instance{
            project.find_instance_by_id(trafo.instance_ref.object_id, trafo.instance_ref.instance_id)
        };
        if (instance == nullptr) {
            continue;
        }
        Domain::Transform3d offset_trafo{instance->get_transformation().get_matrix()};
        offset_trafo.translation().x() = trafo.absolute_offset.x();
        offset_trafo.translation().y() = trafo.absolute_offset.y();

        auto rotation_trafo{Transform3d::Identity()};
        rotation_trafo.rotate(Eigen::AngleAxisd(trafo.rotation_delta, Eigen::Vector3d::UnitZ()));

        instance->set_transformation(Transformation{offset_trafo * rotation_trafo});
        elements.push_back(trafo.instance_ref);
    }

    const BedTrackingChanges changes{m_bed_tracking.update_instances_bed_placement(project, elements)};
    for (const auto& bed_ref : changes.updated_beds) {
        invoke_slicing_input_changed(bed_ref);
    }

    invoke_listeners<ISceneChangedListener>(
        [&](ISceneChangedListener* l)
        { l->on_instance_transformed(m_selected_project_id, elements, TransformState::Completed, changes); }
    );
}

void SceneInteractor::finalize_transform_selection(TransformMemento& memento, bool canceled)
{
    auto& proj = m_projects.find(m_selected_project_id)->second;
    const bool vol_mode = memento.forced_volume_mode || proj.object_selection.mode == SelectionMode::Volume;

    auto selected_elements = proj.object_selection.elements;
    if (memento.forced_volume_mode) {
        selected_elements.clear();
        for (const auto& e : memento.elements)
            selected_elements.push_back(e.first);
    }

    BedTrackingChanges changes;
    if (canceled) {
        for (const auto& [_, e] : memento.elements) {
            const Transformation xform{Transform3d{e.original_xform}};
            if (vol_mode) {
                auto* vol = proj.project.find_volume_by_id(e.element.object_id, e.element.volume_id);
                vol->set_transformation(xform);
            } else {
                auto* inst = proj.project.find_instance_by_id(e.element.object_id, e.element.instance_id);
                inst->set_transformation(xform);
            }
        }

        changes = update_selection_instance_bed_placement(memento.forced_volume_mode);
    }

    invoke_listeners<ISceneChangedListener>(
        [&](ISceneChangedListener* l)
        {
            if (vol_mode)
                l->on_volume_transformed(m_selected_project_id, selected_elements, canceled ? TransformState::Canceled : TransformState::Completed, changes);
            else
                l->on_instance_transformed(m_selected_project_id, selected_elements, canceled ? TransformState::Canceled : TransformState::Completed, changes);
        }
    );

    if (!canceled && !memento.elements.empty()) {
        invoke_listeners<ISceneSelectionChangedListener>(
            [&](ISceneSelectionChangedListener* l)
            { l->on_scene_selection_transformed(m_selected_project_id, proj.object_selection); }
        );
    }

    memento.reset();
}

BedTrackingChanges SceneInteractor::update_selection_instance_bed_placement(bool forced_volume_mode)
{
    BedTrackingChanges changes;
    auto& proj          = m_projects.find(m_selected_project_id)->second;
    const bool vol_mode = forced_volume_mode || proj.object_selection.mode == SelectionMode::Volume;
    if (vol_mode) {
        std::set<size_t> object_ids;
        for (const auto& e : proj.object_selection.elements)
            object_ids.insert(e.object_id);
        for (size_t obj_id : object_ids)
            changes.append(
                m_bed_tracking.update_instances_bed_placement(proj.project, proj.project.find_object_by_id(obj_id)->instances)
            );
    } else {
        changes = m_bed_tracking.update_instances_bed_placement(proj.project, proj.object_selection.elements);
    }
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    return changes;
}

void SceneInteractor::invoke_slicing_input_changed(const Domain::BedRef& bed_instance)
{
    invoke_listeners<ISlicingInputChangedListener>(
        [&](auto listener) { listener->on_slicing_input_changed(bed_instance); }
    );
}

} // namespace Slic3r::Biz::Scene
