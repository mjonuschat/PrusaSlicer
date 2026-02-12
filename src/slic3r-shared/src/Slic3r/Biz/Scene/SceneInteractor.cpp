#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

#include "Slic3r/Biz/Arrange/Arrange.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Types.hpp"

#include "Slic3r/Biz/Arrange/Arrange.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/ModelVolume.hpp"
#include "Slic3r/Biz/Scene/BedFactory.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Math.hpp"
#include "Slic3r/Biz/Algorithms/Bed.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/ModelInstance.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/Scene/SelectionExtents.hpp"

#include <Slic3r/Assert.hpp>
#include <Slic3r/Log.hpp>
#include <Slic3r/Directories.hpp>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <vector>
#include <algorithm>
#include <boost/filesystem/path.hpp>

using Slic3r::Domain::BedContainer;
using Slic3r::Domain::ConstModelInstanceList;
using Slic3r::Domain::Model;
using Slic3r::Domain::ModelInstance;
using Slic3r::Domain::ModelObject;
using Slic3r::Domain::ModelVolume;
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
 * @param xform The relative transformation matrix to check.
 * @return true if the transform contains any rotation component around
 * the X or Y axes, which would change the direction of the Z-axis or
 * any tranlation in z.
 */
static bool changes_z_rotation_or_position(const Eigen::Matrix4d& xform)
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

static Domain::SquareMatrix4d get_wipe_tower_transform(const Domain::BedInstance& bed_instance)
{
    Domain::Transform3d result{bed_instance.transformation.get_matrix()};
    const Vec2d position{bed_instance.wipe_tower.position};
    result.translate(Vec3d{position.x(), position.y(), 0.0});
    result.rotate(
        Eigen::AngleAxisd{Slic3r::deg2rad(bed_instance.wipe_tower.rotation), Vec3d::UnitZ()}
    );

    return result.matrix();
};

static double get_xy_rotation(
    const Transformation& transformation
) {
    const Domain::SquareMatrix3d rotation{transformation.get_rotation_matrix().rotation()};
    return std::atan2(rotation(1, 0), rotation(0, 0));
}

static void set_wipe_tower_transformation(
    const Transformation& transformation,
    Domain::BedInstance& bed_instance
)
{
    if (changes_z_rotation_or_position(transformation.get_matrix().matrix())) {
        return;
    }
    Domain::ModelWipeTower& wipe_tower{bed_instance.wipe_tower};
    wipe_tower.position =
        transformation.get_offset().head<2>() - bed_instance.transformation.get_offset().head<2>();
    const double angle =
        get_xy_rotation(transformation) - get_xy_rotation(bed_instance.transformation);
    wipe_tower.rotation = Slic3r::rad2deg(Slic3r::angle_to_0_2PI(angle));
}

SelectionMode transform_selection_instance_mode(
    const SceneInteractorProjectContext& proj,
    const SceneInteractor::Transform& relative_transform,
    TransformMemento& memento
)
{
    const bool initialize_memento = memento.elements.empty();
    const auto& sel               = proj.object_selection;
    DEBUG_ASSERT(sel.mode == SelectionMode::Instance);

    const bool volume_transform_mode = memento.forced_volume_mode || changes_z_rotation_or_position(relative_transform);

    if (initialize_memento) {
        memento.elements.reserve(sel.elements.size());

        for (const auto& e : sel.elements) {
            if (!e.is_wipe_tower()) {
                continue;
            }
            Domain::BedInstance* bed_instance =
                proj.project.find_bed_instance_by_id(e.wipe_tower_id.bed_instance_id);
            ASSERT(bed_instance);
            memento.elements.insert({e, {e, get_wipe_tower_transform(*bed_instance)}});
        }
    }

    for (const auto& [_, e] : memento.elements) {
        if (!e.element.is_wipe_tower()) {
            continue;
        }
        Domain::BedInstance* bed_instance =
            proj.project.find_bed_instance_by_id(e.element.wipe_tower_id.bed_instance_id);
        ASSERT(bed_instance);
        set_wipe_tower_transformation(
            transform_product(e.original_xform, relative_transform),
            *bed_instance
        );
    }

    std::set<size_t> object_ids;
    for (const auto& e : sel.elements) {
        ModelInstance* inst = proj.project.find_instance_by_id(e.object_id, e.instance_id);
        if (!inst) {
            continue;
        }
        if (volume_transform_mode) {
            object_ids.insert(inst->get_object()->id().id);
        } else {
            if (initialize_memento) {
                memento.elements.insert({e, {e, inst->get_matrix().matrix()}});
            }
            inst->set_transformation(
                transform_product(memento.elements[e].original_xform, relative_transform)
            );
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

    auto& project = m_workbench.project(index);
    if (m_projects.count(index) == 0) {
        BedSelection selection{};
        selection.on_change = [this, index](const BedSelection& bed_selection)
        {
            invoke_listeners<ISelectedBedInstancesChangedListener>(
                [&](auto* l) { l->on_selected_bed_instances_changed(index, bed_selection); }
            );
        };
        m_projects.emplace(index, SceneInteractorProjectContext{project, selection});
    }
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
    ASSERT(!selection.empty());
    if (selection.last_selected_bed().config_container_id != container_id) {
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

void SceneInteractor::set_object_selection(const ObjectSelection& raw_selection)
{
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());

    ObjectSelection selection = raw_selection;
    normalize_object_selection(selection);

    auto& project_context = it->second;
    ObjectSelection sel{.mode = selection.mode};
    for (const auto& e : selection.elements) {
        if (e.has_instance() || e.is_wipe_tower()) {
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
    update_selection_bounding_box();
}

static Domain::SquareMatrix3d reset_shear(const Domain::SquareMatrix3d& matrix)
{
    const Eigen::JacobiSVD<Domain::SquareMatrix3d> svd(
        matrix,
        Eigen::ComputeFullU | Eigen::ComputeFullV
    );

    const Domain::SquareMatrix3d rotation{svd.matrixU() * svd.matrixV().transpose()};
    const Domain::SquareMatrix3d symetric_scale{
        svd.matrixV() * svd.singularValues().asDiagonal() * svd.matrixV().transpose()
    };

    const Domain::SquareMatrix3d scale{symetric_scale.diagonal().asDiagonal()};

    return rotation * scale;
}

static bool has_shear(const Domain::SquareMatrix4d& matrix)
{
    Domain::SquareMatrix4d no_shear{matrix};
    no_shear.block(0, 0, 3, 3) = reset_shear(no_shear.block(0, 0, 3, 3));
    return ((no_shear - matrix).cwiseAbs().array() > 1e-6 * Domain::SquareMatrix4d::Ones().array())
        .any();
}

Domain::ElementRefs SceneInteractor::selected_volumes_with_shear() const
{
    ASSERT(m_selected_project_id != Domain::INVALID_ID);
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());

    const ObjectSelection& selection{it->second.object_selection};

    Domain::ElementRefs result;
    for (const Domain::ElementRef& element : selection.elements) {
        const Domain::ModelInstance* instance{
            m_workbench.project(m_selected_project_id)
                .find_instance_by_id(element.object_id, element.instance_id)
        };

        if (element.has_volume()) {
            const Domain::ElementRef ref{element.object_id, 0, element.volume_id};
            const Domain::ModelVolume* volume{m_workbench.project(m_selected_project_id)
                                                  .find_volume_by_id(ref.object_id, ref.volume_id)};
            if (has_shear(volume->get_matrix().matrix())) {
                result.push_back(ref);
            }
        } else {
            ASSERT(element.has_instance());
            for (const Domain::ModelVolume* volume : instance->get_object()->volumes) {
                const Domain::ElementRef ref{element.object_id, 0, volume->id().id};
                if (has_shear(volume->get_matrix().matrix())) {
                    result.push_back(ref);
                }
            }
        }
    }

    return result;
}

std::set<SelectionReferenceFrame> SceneInteractor::object_selection_reference_frame_options() const
{
    using SelectionReferenceFrame::Bed;
    using SelectionReferenceFrame::Instance;
    using SelectionReferenceFrame::Volume;

    ASSERT(m_selected_project_id != Domain::INVALID_ID);
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());

    const ObjectSelection& selection{it->second.object_selection};

    switch (selection.state()) {
    case SelectionState::SingleVolume: {
        if (!selected_volumes_with_shear().empty()) {
            return {Bed, Instance};
        }
        return {Bed, Instance, Volume};
    }
    case SelectionState::WholeInstance: {
        ASSERT(!selection.empty());
        return {Bed, Instance};
    }
    default:
        return {Bed};
    }
}

SelectionReferenceFrame SceneInteractor::object_selection_reference_frame() const {
    ASSERT(m_selected_project_id != Domain::INVALID_ID);
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());

    return it->second.object_selection_reference_frame;
}

bool SceneInteractor::reload_object_selection_reference_frame(SelectionReferenceFrame preferred_frame)
{
    ASSERT(m_selected_project_id != Domain::INVALID_ID);
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());

    const SelectionReferenceFrame previous{it->second.object_selection_reference_frame};
    const std::set<SelectionReferenceFrame> options{object_selection_reference_frame_options()};
    if (options.contains(preferred_frame)) {
        it->second.object_selection_reference_frame = preferred_frame;
    } else {
        it->second.object_selection_reference_frame = SelectionReferenceFrame::Bed;
    }

    const bool changed{previous != it->second.object_selection_reference_frame};
    if (changed) {
        update_selection_bounding_box();
    }
    return changed;
}

void SceneInteractor::normalize_object_selection(ObjectSelection& selection) const
{
    ASSERT(m_selected_project_id != Domain::INVALID_ID);
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());

    selection.mode = SelectionMode::Volume;
    if (selection.elements.empty())
        return;

    // verify if promoting to Instance mode is needed
    const auto inst_id = selection.elements.front().instance_id;
    bool requires_instance_mode = std::any_of(
        selection.elements.begin(),
        selection.elements.end(),
        [inst_id](const auto& e) { return e.volume_id == 0 || e.instance_id != inst_id; }
    );

    if (!requires_instance_mode) {
        // are all volumes of a single instance selected ?
        bool single_instance = std::all_of(selection.elements.begin(), selection.elements.end(),
            [inst_id](const auto& e) { return e.instance_id == inst_id; });
        if (single_instance) {
            const auto object = it->second.project.find_object_by_id(selection.elements.front().object_id);
            if (object->volumes.size() == selection.elements.size())
                requires_instance_mode = true;
        }
    }

    if (requires_instance_mode) {
        selection.mode = SelectionMode::Instance;
        std::unordered_set<Domain::ElementRef> unique_inst_elements;

        for (const auto& e : selection.elements) {
            if (e.is_wipe_tower()) {
                unique_inst_elements.insert(e);
                continue;
            }
            unique_inst_elements.insert(Domain::ElementRef{e.object_id, e.instance_id, 0});
        }

        selection.elements.clear();
        selection.elements.insert(selection.elements.end(), unique_inst_elements.begin(), unique_inst_elements.end());
    }
}

void SceneInteractor::clear_object_selection()
{
    set_object_selection(ObjectSelection());
}

void SceneInteractor::modify_selection(const std::function<void(ObjectSelection&)>& modifier)
{
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());
    auto& selection = it->second.object_selection;
    modifier(selection);
    DEBUG_ASSERT(selection.is_valid());
    update_selection_bounding_box();
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

void SceneInteractor::new_object_from_mesh(TriangleMesh&& mesh, const std::string& name) {
    UpdateObjectFn update_object = [&name](ModelObject& object) {
        object.name = name;
        object.volumes.front()->name = name;
        //for (Domain::ModelVolume* volume : object.volumes)
        //    volume->name = name;
    };
    new_object_from_mesh(std::move(mesh), m_selected_project_id, update_object);
}

void SceneInteractor::new_object_from_mesh(TriangleMesh&& mesh, Domain::SelectionId project_id, UpdateObjectFn update_object)
{
    auto& project = m_workbench.project(project_id);
    auto& obj     = *project.model().add_object();
    auto& vol     = *Algorithms::ModelObject::add_volume(&obj, std::move(mesh));
    auto& inst    = *obj.add_instance();
    update_object(obj);

    ASSERT(Algorithms::ModelObject::are_volumes_sorted(&obj));

    const Domain::ElementRefs updated{{obj.id().id, inst.id().id}};
    // const Domain::ElementRefs updated_vols{{obj.id().id, inst.id().id, vol.id().id}};
    auto changes = m_bed_tracking.update_instances_bed_placement(project, updated);
    if (project.file_name().empty()) {
        const boost::filesystem::path filename_path(vol.name);
        const std::string stem_name = filename_path.stem().string();
        project.set_file_name(stem_name);
    }

    for (const auto& bed_ref : changes.updated_beds) {
        invoke_slicing_input_changed(bed_ref);
    }
    invoke_listeners<ISceneChangedListener>([&](auto* l) {
        l->on_instance_added(m_selected_project_id, updated);
    });

    if (!changes.updated_instances.empty()) {
        invoke_listeners<ISceneChangedListener>(
            [&](ISceneChangedListener* l)
            {
                l->on_instances_last_bed_updated(
                    {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                );
            }
        );
    }

    // TODO: need to send project_id for set selection (for @JanBartipan)
    set_object_selection({ SelectionMode::Instance, updated });
}

void SceneInteractor::add_new_objects(const std::vector<Domain::ModelObject*>& objects)
{
    auto& project = m_workbench.project(m_selected_project_id);

    Domain::ModelObjectPtrs new_objects;
    for (Domain::ModelObject* object : objects) {
        Algorithms::ModelObject::sort_volumes(object);
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

    Algorithms::ModelObject::sort_volumes(&obj);

    invoke_listeners<ISceneChangedListener>(
        [&](auto* l) { l->on_volume_added(m_selected_project_id, updated); }
    );

    set_object_selection({SelectionMode::Volume, updated});
    update_elements_bed_placement(sel.elements, sel.mode == SelectionMode::Volume);
}

void SceneInteractor::add_volume(
    Domain::SelectionId project_id,
    Domain::SelectionId instance_id,
    const std::function<Domain::ModelVolume*(Domain::ModelObject&)>& factory
)
{
    auto& p = m_workbench.project(project_id);
    auto* instance = p.find_instance_by_id(instance_id);
    ASSERT(instance != nullptr);
    auto* obj = instance->get_object();

    auto* volume = factory(*obj);

    // verify that the factory added volume to the object
    ASSERT(std::ranges::find(obj->volumes, volume) != obj->volumes.end());

    Algorithms::ModelObject::sort_volumes(obj);

    Domain::ElementRefs updated = {
        Domain::ElementRef(obj->id().id, instance_id, volume->id().id)
    };

    invoke_listeners<ISceneChangedListener>([&](auto* l) {
        l->on_volume_added(project_id, updated);
    });

    set_object_selection({ SelectionMode::Volume, updated});

    auto changes = m_bed_tracking.update_instances_bed_placement(p, updated);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);
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

    if (!changes.updated_instances.empty()) {
        invoke_listeners<ISceneChangedListener>(
            [&](ISceneChangedListener* l)
            {
                l->on_instances_last_bed_updated(
                    {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                    );
            }
            );
    }

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
    }

    auto changes = m_bed_tracking.update_instances_bed_placement(project, updated);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

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
        removed_ids.emplace_back(id.object_id, id.instance_id, id.volume_id);
        updated_ids.emplace_back(id.object_id, id.instance_id, volume.id().id);
    }

    std::sort(object_ids.begin(), object_ids.end());
    object_ids.erase(std::unique(object_ids.begin(), object_ids.end()), object_ids.end());
    for (size_t object_id : object_ids)
        project.find_object_by_id(object_id)->invalidate_bounding_box();

    invoke_listeners<ISceneChangedListener>(
        [&removed_ids, &updated_ids, project_id = m_selected_project_id](auto* l)
        {
            l->on_volume_removed(project_id, removed_ids);
            l->on_volume_added(project_id, updated_ids);
        }
    );
    
    set_object_selection({SelectionMode::Volume, updated_ids});
    auto changes = m_bed_tracking.update_instances_bed_placement(project, updated_ids);
    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);
}

void SceneInteractor::modify_facets_annotations(
    const Domain::ElementRefs& volume_refs,
    const std::function<bool(const Domain::ElementRef&, ModelVolume&)>& modifier
)
{
    if (volume_refs.empty()) {
        return;
    }

    Project& project = m_workbench.project(m_selected_project_id);

    Domain::ElementRefs instances_to_update;
    for (const Domain::ElementRef& volume_ref : volume_refs) {
        ModelObject* model_object = project.find_object_by_id(volume_ref.object_id);
        ModelVolume* model_volume =
            project.find_volume_by_id(volume_ref.object_id, volume_ref.volume_id);

        ASSERT(model_object != nullptr);
        for (const ModelInstance* model_instance : model_object->instances) {
            if (modifier(volume_ref, *model_volume)) {
                instances_to_update.emplace_back(
                    volume_ref.object_id,
                    model_instance->id().id,
                    volume_ref.volume_id
                );
            }
        }
    }

    BedTrackingChanges changes =
        m_bed_tracking.update_instances_bed_placement(project, instances_to_update);
    for (const Domain::BedRef& bed_ref : changes.updated_beds) {
        this->invoke_slicing_input_changed(bed_ref);
    }
}

void SceneInteractor::modify_layer_height_profile(
    const Domain::ElementRef& object_ref,
    const std::function<void(ModelObject&)>& modifier
)
{
    Project& project          = m_workbench.project(m_selected_project_id);
    ModelObject* model_object = project.find_object_by_id(object_ref.object_id);
    ASSERT(model_object != nullptr);

    modifier(*model_object);

    Domain::ElementRefs instance_refs;
    for (const ModelInstance* model_instance : model_object->instances) {
        instance_refs.emplace_back(object_ref.object_id, model_instance->id().id);
    }

    BedTrackingChanges changes =
        m_bed_tracking.update_instances_bed_placement(project, instance_refs);
    for (const Domain::BedRef& bed_ref : changes.updated_beds) {
        this->invoke_slicing_input_changed(bed_ref);
    }
}

void SceneInteractor::modify_layer_config_ranges(
    const Domain::ElementRef& object_ref,
    const std::function<void(ModelObject&)>& modifier
)
{
    Project& project          = m_workbench.project(m_selected_project_id);
    ModelObject* model_object = project.find_object_by_id(object_ref.object_id);
    ASSERT(model_object != nullptr);

    modifier(*model_object);

    Domain::ElementRefs instance_refs;
    for (const ModelInstance* model_instance : model_object->instances) {
        instance_refs.emplace_back(object_ref.object_id, model_instance->id().id);
    }

    BedTrackingChanges changes =
        m_bed_tracking.update_instances_bed_placement(project, instance_refs);
    for (const Domain::BedRef& bed_ref : changes.updated_beds) {
        this->invoke_slicing_input_changed(bed_ref);
    }
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

        Domain::ElementRef stay_el{sel_object_id, old_object->instances[0]->id().id};
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

    for (const Domain::ElementRef& element : to_remove) {
        if (element.is_wipe_tower()) {
            return std::nullopt;
        }
    }

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

//        normalize_single_volume_object(*object);

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

        changes = m_bed_tracking.update_instances_bed_placement(project, to_remove);
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

    if (!changes.updated_instances.empty()) {
        invoke_listeners<ISceneChangedListener>(
            [&](ISceneChangedListener* l)
            {
                l->on_instances_last_bed_updated(
                    {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                );
            }
        );
    }

    set_object_selection(new_selection);

    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    return last_solid_part_name;
}

bool SceneInteractor::delete_object(Domain::ModelObject* object)
{
    // ToDo: Check if object cen be delete (for example if this object ia a part of cut set)
    // return false if delete is impossible

    Domain::Project& project = m_workbench.project(m_selected_project_id);
    Domain::Model& model     = project.model();

    ObjectSelection::ElementRefs to_remove;
    BedTrackingChanges changes;

    // Remove object instances from its object and scene
    for (auto instance : object->instances) {
        remove_instance_from_bed(project, instance, changes);
        to_remove.push_back({ object->id().id, instance->id().id });
    }
    // Remove object from the model
    model.delete_object(object);

    // Notify listeners on changes

    invoke_listeners<ISceneChangedListener>(
        [&](auto* l) { l->on_instance_removed(m_selected_project_id, to_remove); }
    );

    for (const auto& bed_ref : changes.updated_beds)
        invoke_slicing_input_changed(bed_ref);

    return true;
}

void SceneInteractor::prepare_added_project(Domain::SelectionId project_id)
{
    Domain::Project& project = m_workbench.project(project_id);
    m_bed_tracking.update_instances_bed_placement(project);
    for (auto& cc : project.config_containers()) {
        update_config_container_bed(project_id, cc->id().id);
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

    if (!changes.updated_instances.empty()) {
        invoke_listeners<ISceneChangedListener>(
            [&](ISceneChangedListener* l)
            {
                l->on_instances_last_bed_updated(
                    {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                );
            }
        );
    }

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

    if (!changes.updated_instances.empty()) {
        invoke_listeners<ISceneChangedListener>(
            [&](ISceneChangedListener* l)
            {
                l->on_instances_last_bed_updated(
                    {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                );
            }
        );
    }

    invoke_listeners<ISceneBedInstanceChangedListener>(
        [&](auto* l) { l->on_bed_instance_updated(m_selected_project_id, bed_refs); }
    );
}

void SceneInteractor::remove_bed_instance(const Domain::BedRef& instance, bool allow_to_remove_last_one)
{
    auto& project               = m_projects.find(m_selected_project_id)->second.project;
    Domain::ConfigContainer* cc = project.find_config_container(instance.config_container_id);
    ASSERT(cc != nullptr);
    auto* bed_inst = Domain::find_by_id(cc->bed_instances(), instance.instance_id);

    // model instances to be removed toghether with the bed instance containing them
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
        if (allow_to_remove_last_one) {
            if (cc->bed_instances().empty()) {
                auto& ccs = project.config_containers();
                ASSERT(ccs.size() > 1);
                auto it = std::find_if(
                    ccs.begin(),
                    ccs.end(),
                    [config_container_id = instance.config_container_id](const auto& cc_ptr)
                    { return cc_ptr->id().id == config_container_id; }
                );
                if (++it == ccs.end())
                    it = ccs.begin();
                selection.select_one(
                    {it->get()->id().id, it->get()->bed_instances().front()->id().id}, CameraActionOnBedSelection::CenterOnBed
                );
            }
        } else {
            // ensure one bed instance is selected
            ASSERT(!cc->bed_instances().empty());
            selection.select_one({cc->id().id, cc->bed_instances().front()->id().id});
        }
    }

    invoke_listeners<ISceneBedInstanceChangedListener>(
        [&](auto* l) { l->on_bed_instance_updated(m_selected_project_id, updated); }
    );

    auto changes = m_bed_tracking.update_instances_bed_placement(project, updated_instances);
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

    if (!changes.updated_instances.empty()) {
        invoke_listeners<ISceneChangedListener>(
            [&](ISceneChangedListener* l)
            {
                l->on_instances_last_bed_updated(
                    {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                );
            }
        );
    }

    if (!allow_to_remove_last_one && cc->bed_instances().empty()) {
        add_bed_instance(cc->id().id);
    }

    // temporary object selection containing model instances to be removed
    ObjectSelection model_instances_to_remove;
    model_instances_to_remove.elements.reserve(insts.size());
    for (const auto inst : insts) {
        model_instances_to_remove.elements.emplace_back(Domain::ElementRef{ inst->get_object()->id().id, inst->id().id });
    }

    if (!model_instances_to_remove.empty()) {
        // removes the model instances from the current object selection
        ObjectSelection curr_scene_selection = object_selection();
        for (const auto& e : model_instances_to_remove.elements) {
            curr_scene_selection.remove(e);
        }

        // delete the model instances from the project
        set_object_selection(model_instances_to_remove);
        delete_selected_elements();
        // restore modified object selection
        set_object_selection(curr_scene_selection);
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

    if (!changes.updated_instances.empty()) {
        invoke_listeners<ISceneChangedListener>(
            [&](ISceneChangedListener* l)
            {
                l->on_instances_last_bed_updated(
                    {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                );
            }
        );
    }
}

void SceneInteractor::update_config_container_bed(Domain::SelectionId project_id, Domain::SelectionId config_container_id)
{
    Domain::Project& project = m_workbench.project(project_id);
    Domain::ConfigContainer* config_container{project.find_config_container(config_container_id)};
    if (config_container == nullptr)
        return;

    Domain::Bed& bed{
        get_or_create_bed(
            project.bed_container(), *config_container, resources_dir(), project_id, config_container_id,
            [this](Domain::SelectionId project_id, Domain::SelectionId config_container_id) {
                return (m_preset_visual_getter != nullptr) ?
                    m_preset_visual_getter->system_preset_bed_shape(project_id, config_container_id) : Domain::Vec2ds();
            }
        )
    };

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
        for (const auto* mi : project.unplaced_model_instances()) {
            changed_instances.emplace_back(mi->get_object()->id().id, mi->id().id);
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
                    project_id,
                    updated,
                    TransformState::Completed,
                    changes
                );
            }
        );

        invoke_listeners<ISceneBedInstanceChangedListener>(
            [&](auto* l) { l->on_bed_instance_updated(project_id, bed_refs); }
        );

        if (!changes.updated_instances.empty()) {
            invoke_listeners<ISceneChangedListener>(
                [&](ISceneChangedListener* l)
                {
                    l->on_instances_last_bed_updated(
                        {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                    );
                }
            );
        }
    }
}

void SceneInteractor::on_preset_selection_changed(Domain::SelectionId project_id, Domain::SelectionId config_container_id, Preset::PresetItemType type)
{
    if (type != Preset::PresetItemType::PrinterPreset) {
        return;
    }

    update_config_container_bed(project_id, config_container_id);
}

void SceneInteractor::on_preset_value_changed(Domain::SelectionId project_id, Domain::SelectionId config_container_id, const Domain::ConfigItem& item)
{
    const std::vector<std::string> bed_related_keys{
        "bed_shape",
        "max_print_height",
        "bed_custom_model",
        "bed_custom_texture",
    };
    if (std::ranges::find(bed_related_keys, item.def().name) != bed_related_keys.end()) {
        update_config_container_bed(project_id, config_container_id);
    }
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

void SceneInteractor::set_element_transforms(const SceneInteractor::ElementTransforms& transforms)
{
    Domain::ElementRefs elements;

    Project& project{m_workbench.project(m_selected_project_id)};

    for (const auto& [element, transform] : transforms) {
        elements.push_back(element);
        if (element.has_volume()) {
            ASSERT(element.has_volume());
            ASSERT(!element.has_instance());
            Domain::ModelVolume* volume{
                project.find_volume_by_id(element.object_id, element.volume_id)
            };
            volume->set_transformation(Transformation{Transform3d{transform}});
        } else {
            ASSERT(!element.has_volume());
            ASSERT(element.has_instance());
            Domain::ModelInstance* instance{
                project.find_instance_by_id(element.object_id, element.instance_id)
            };
            instance->set_transformation(Transformation{Transform3d{transform}});
        }
    }

    update_selection_bounding_box();

    const BedTrackingChanges changes = update_elements_bed_placement(elements, true);
    invoke_listeners<ISceneChangedListener>(
        [&](ISceneChangedListener* l)
        {
            l->on_volume_transformed(
                m_selected_project_id,
                elements,
                TransformState::Completed,
                changes
            );
            l->on_instance_transformed(
                m_selected_project_id,
                elements,
                TransformState::Completed,
                changes
            );
        }
    );
    invoke_listeners<ISceneSelectionChangedListener>(
        [&](ISceneSelectionChangedListener* l)
        { l->on_scene_selection_transformed(m_selected_project_id, object_selection()); }
    );

    if (!changes.updated_instances.empty()) {
        invoke_listeners<ISceneChangedListener>(
            [&](ISceneChangedListener* l)
            {
                l->on_instances_last_bed_updated(
                    {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                );
            }
        );
    }
}

void SceneInteractor::transform_selection(const Transform& relative_transform, bool place_on_bed)
{
    TransformMemento memento;
    transform_selection(relative_transform, memento, place_on_bed);
    finalize_transform_selection(memento, false);
}

void SceneInteractor::transform_selection(
    const SquareMatrix4d& relative_transform,
    TransformMemento& memento,
    bool place_on_bed
)
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
            for (const auto& e : memento.elements) {
                selection.elements.push_back(e.first);
            }
        }
    } else
        transform_selection_volume_mode(proj, relative_transform, memento);

    std::optional<SelectionExtents> bounding_box{get_selection_extents(
        m_selected_project_id,
        proj.object_selection,
        *this,
        m_workbench
    )};

    if (bounding_box
        && place_on_bed
        && proj.object_selection.state() != SelectionState::SingleVolume
        && bounding_box->is_floating()
        && selection.mode == SelectionMode::Volume)
    {
        for (const auto& e : selection.elements) {
            if (!e.has_instance() || !e.has_volume()) {
                continue;
            }
            const auto* instance{proj.project.find_instance_by_id(e.object_id, e.instance_id)};
            const auto instance_matrix{instance->get_matrix()};
            ModelVolume* volume = proj.project.find_volume_by_id(e.object_id, e.volume_id);
            if (!volume) {
                continue;
            }
            Domain::Transform3d transformation{Domain::Transform3d::Identity()};
            transformation.translate(Domain::Vec3d{0.0, 0.0, -bounding_box->min_z()});

            const Domain::Transform3d volume_relative_transform =
                instance_matrix.inverse() * transformation * instance_matrix;

            volume->set_transformation(
                Transformation{volume_relative_transform * volume->get_matrix()}
            );
        }
        bounding_box->reset_z();
    }

    update_selection_bounding_box(bounding_box);


    const BedTrackingChanges changes = update_elements_bed_placement(
        proj.object_selection.elements,
        selection.mode == SelectionMode::Volume || memento.forced_volume_mode
    );

    for (const auto& [_, e] : memento.elements) {
        if (e.element.is_wipe_tower()) {
            invoke_listeners<ISceneChangedListener>(
                [&](auto listener) { listener->on_wipe_tower_moved(e.element.wipe_tower_id); }
            );
        }
    }
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

    if (!changes.updated_instances.empty()) {
        invoke_listeners<ISceneChangedListener>(
            [&](ISceneChangedListener* l)
            {
                l->on_instances_last_bed_updated(
                    {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                );
            }
        );
    }
}

void SceneInteractor::transform_instances(const std::vector<Arrange::InstanceTransform2D>& transformations)
{
    Project& project{m_projects.find(m_selected_project_id)->second.project};

    std::vector<Domain::ElementRef> elements;
    for (const Arrange::InstanceTransform2D& trafo : transformations) {
        if (trafo.instance_ref.is_wipe_tower()) {
            const Domain::SlicingId wipe_tower_id{trafo.instance_ref.wipe_tower_id};
            Domain::BedInstance* bed_instance{
                project.find_bed_instance_by_id(wipe_tower_id.bed_instance_id)
            };
            if (!bed_instance) {
                continue;
            }

            const Print::WipeTowerGeometry* geometry{
                wipe_tower_geometry(wipe_tower_id.bed_instance_id)
            };
            if (!geometry) {
                continue;
            }

            Domain::Transform3d transformation{Domain::Transform3d::Identity()};
            transformation.translate(
                Vec3d{trafo.absolute_offset.x(), trafo.absolute_offset.y(), 0.0}
            );
            transformation.rotate(
                Eigen::AngleAxisd(
                    trafo.rotation_delta + Slic3r::deg2rad(bed_instance->wipe_tower.rotation),
                    Eigen::Vector3d::UnitZ()
                )
            );

            set_wipe_tower_transformation(Domain::Transformation{transformation}, *bed_instance);
            invoke_listeners<ISceneChangedListener>(
                [&](auto listener) { listener->on_wipe_tower_moved(wipe_tower_id); }
            );
        } else {
            ModelInstance* instance{
                project.find_instance_by_id(trafo.instance_ref.object_id, trafo.instance_ref.instance_id)
            };
            if (instance == nullptr) {
                continue;
            }
            Domain::Transform3d offset_trafo{instance->get_transformation().get_matrix()};
            offset_trafo.translation().x() = trafo.absolute_offset.x();
            offset_trafo.translation().y() = trafo.absolute_offset.y();

            auto rotation_trafo{Domain::Transform3d::Identity()};
            rotation_trafo.rotate(Eigen::AngleAxisd(trafo.rotation_delta, Eigen::Vector3d::UnitZ()));

            instance->set_transformation(Transformation{offset_trafo * rotation_trafo});
        }
        elements.push_back(trafo.instance_ref);
    }

    update_selection_bounding_box();

    const BedTrackingChanges changes{m_bed_tracking.update_instances_bed_placement(project, elements)};

    for (const auto& bed_ref : changes.updated_beds) {
        invoke_slicing_input_changed(bed_ref);
    }

    invoke_listeners<ISceneChangedListener>(
        [&](ISceneChangedListener* l)
        { l->on_instance_transformed(m_selected_project_id, elements, TransformState::Completed, changes); }
    );
}

static Vec2d rotate_around(
    const Vec2d& p,
    const Vec2d& center,
    double angle_rad)
{
    Eigen::Rotation2Dd rotation(angle_rad);
    return rotation * (p - center) + center;
}

static Domain::BoundingBox2d get_wipe_tower_bounding_box(
    const Print::WipeTowerGeometry& wipe_tower_geometry,
    const Domain::BedInstance& bed_instance
)
{
    const Vec2d position{
        bed_instance.wipe_tower.position + bed_instance.transformation.get_offset().head<2>()
    };
    const double rotation{
        bed_instance.wipe_tower.rotation + get_xy_rotation(bed_instance.transformation)
    };

    const double block_width{wipe_tower_geometry.width};
    double block_depth{wipe_tower_geometry.fallback_depth};

    const double width{wipe_tower_geometry.width + 2 * wipe_tower_geometry.brim_width};
    double height{wipe_tower_geometry.fallback_height};
    double depth{wipe_tower_geometry.fallback_depth + 2 * wipe_tower_geometry.brim_width};

    if (!wipe_tower_geometry.depths.empty()) {
        block_depth = wipe_tower_geometry.depths.front().depth;
        depth  = wipe_tower_geometry.depths.front().depth + 2 * wipe_tower_geometry.brim_width;
        height = wipe_tower_geometry.depths.back().z;
    }

    const double cone_depth{
        2.0 * height * std::tan(Slic3r::deg2rad(wipe_tower_geometry.cone_angle / 2.0))
        + 2 * wipe_tower_geometry.brim_width
    };
    if (depth < cone_depth) {
        depth = cone_depth;
    }

    const Vec2d front_left{
        position.x() - (width - block_width) / 2.0,
        position.y() - (depth - block_depth) / 2.0
    };
    const Vec2d front_right{
        front_left.x() + width,
        front_left.y()
    };
    const Vec2d back_right{
        front_left.x() + width,
        front_left.y() + depth
    };
    const Vec2d back_left{
        front_left.x(),
        front_left.y() + depth
    };

    Domain::Vec2ds square{
        front_left,
        front_right,
        back_right,
        back_left
    };

    Domain::BoundingBox2d bounding_box;
    for (const Vec2d& point : square) {
        const Vec2d rotated_point{rotate_around(
            point,
            position,
            Slic3r::deg2rad(rotation)
        )};
        bounding_box = Biz::Algorithms::BoundingBox::merge(bounding_box, rotated_point);
    }
    return bounding_box;
}

void SceneInteractor::finalize_transform_selection(TransformMemento& memento, bool canceled)
{
    auto& proj = m_projects.find(m_selected_project_id)->second;

    std::vector<TransformMemento::Element> removed_wipe_towers;
    std::vector<TransformMemento::Element> wipe_towers_outside;
    for (const auto& [_, e] : memento.elements) {
        if (!e.element.is_wipe_tower()) {
            continue;
        }
        const Domain::BedInstance* bed_instance{
            proj.project.find_bed_instance_by_id(e.element.wipe_tower_id.bed_instance_id)
        };
        if (!bed_instance) {
            removed_wipe_towers.push_back(e);
            continue;
        }
        const auto it{proj.wipe_tower_geometries.find(e.element.wipe_tower_id.bed_instance_id)};
        if (it == proj.wipe_tower_geometries.end()){
            removed_wipe_towers.push_back(e);
            continue;
        }

        // Here the wipe tower geometry is not updated yet. The backend callback comes after this.
        // Make sure to check with the position and rotation after transformation.
        // Also, the wipe tower position and rotation are bed relative.
        Print::WipeTowerGeometry geometry{it->second};
        const Domain::BoundingBox2d bounding_box{get_wipe_tower_bounding_box(geometry, *bed_instance)};
        // temporary placeholder until the convex hull of the wipe tower is implemented
        const Domain::Vec2ds convex_hull;
        using Algorithms::Bed::BedContainmentState;
        const auto& bed = bed_instance->bed.get();
        std::optional<AABBMesh> bed_aabb_mesh;
        if (bed.type() == Domain::BedType::Custom)
            bed_aabb_mesh.emplace(Algorithms::Bed::bed_contour_as_aabb_mesh(bed));
        AABBMesh* bed_aabb_mesh_ptr = bed_aabb_mesh.has_value() ? &*bed_aabb_mesh : nullptr;
        Algorithms::Bed::BedInstanceCollisionData bi_collision_data(
            *bed_instance,
            bed_aabb_mesh.has_value() ? &*bed_aabb_mesh : nullptr
        );
        const BedContainmentState contains{ Algorithms::Bed::contains_2d(bi_collision_data, bounding_box, convex_hull) };
        if (contains != BedContainmentState::Inside) {
            wipe_towers_outside.push_back(e);
        }
    };

    const bool vol_mode = memento.forced_volume_mode || proj.object_selection.mode == SelectionMode::Volume;

    auto selected_elements = proj.object_selection.elements;
    if (memento.forced_volume_mode) {
        selected_elements.clear();
        for (const auto& e : memento.elements)
            selected_elements.push_back(e.first);
    }

    std::vector<TransformMemento::Element> wipe_towers_to_restore;
    if (canceled) {
        for (const auto& [_, e] : memento.elements) {
            if (!e.element.is_wipe_tower()) {
                continue;
            }
            wipe_towers_to_restore.push_back(e);
        }
    } else {
        wipe_towers_to_restore.insert(
            wipe_towers_to_restore.end(),
            removed_wipe_towers.begin(),
            removed_wipe_towers.end()
        );
        wipe_towers_to_restore.insert(
            wipe_towers_to_restore.end(),
            wipe_towers_outside.begin(),
            wipe_towers_outside.end()
        );
    }

    for (const TransformMemento::Element& e : wipe_towers_to_restore) {
        const Transformation xform{Transform3d{e.original_xform}};
        Domain::BedInstance* bed_instance =
            proj.project.find_bed_instance_by_id(e.element.wipe_tower_id.bed_instance_id);
        ASSERT(bed_instance);
        set_wipe_tower_transformation(xform, *bed_instance);
        invoke_listeners<ISceneChangedListener>(
            [&](auto listener) { listener->on_wipe_tower_moved(e.element.wipe_tower_id); }
        );
    }

    for (const TransformMemento::Element& e : removed_wipe_towers) {
        proj.object_selection.remove(Domain::ElementRef{e.element.wipe_tower_id});
    }
    if (!removed_wipe_towers.empty()) {
        invoke_listeners<ISceneSelectionChangedListener>(
            [&](auto* l)
            { l->on_scene_selection_changed(m_selected_project_id, proj.object_selection); }
        );
    }

    BedTrackingChanges changes;
    if (canceled || !wipe_towers_outside.empty()) {
        for (const auto& [_, e] : memento.elements) {
            const Transformation xform{Transform3d{e.original_xform}};
            if (e.element.is_wipe_tower()) {
                continue;
            }
            if (vol_mode) {
                auto* vol = proj.project.find_volume_by_id(e.element.object_id, e.element.volume_id);
                vol->set_transformation(xform);
            } else {
                auto* inst = proj.project.find_instance_by_id(e.element.object_id, e.element.instance_id);
                inst->set_transformation(xform);
            }
        }

        changes = update_elements_bed_placement(proj.object_selection.elements, vol_mode);
    }

    update_selection_bounding_box();

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

    if (!changes.updated_instances.empty()) {
        invoke_listeners<ISceneChangedListener>(
            [&](ISceneChangedListener* l)
            {
                l->on_instances_last_bed_updated(
                    {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                );
            }
        );
    }

    memento.reset();
}

void SceneInteractor::on_removed_config_container(Domain::Project& project)
{
    layout_after_project_load(project);
}

void SceneInteractor::on_wipe_tower_geometry_changed(
    Print::OptWipeTowerGeometry wipe_tower,
    const Domain::SlicingId slicing_id
)
{
    if (!wipe_tower) {
        remove_wipe_tower(slicing_id);
        return;
    }
    change_wipe_tower(*wipe_tower, slicing_id);
}

void SceneInteractor::remove_wipe_tower(const Domain::SlicingId slicing_id)
{
    const auto& project_it{m_projects.find(slicing_id.project_id)};
    if (project_it == m_projects.end()) {
        return;
    }
    SceneInteractorProjectContext& context{project_it->second};
    context.wipe_tower_geometries.erase(slicing_id.bed_instance_id);

    if (object_selection().is_selected(Domain::ElementRef{slicing_id})) {
        update_selection_bounding_box();
    }
    invoke_listeners<ISceneChangedListener>([&](auto listener)
                                            { listener->on_wipe_tower_removed(slicing_id); });
}

void SceneInteractor::change_wipe_tower(
    const Print::WipeTowerGeometry& wipe_tower,
    const Domain::SlicingId slicing_id
)
{
    const auto& project_it{m_projects.find(m_selected_project_id)};
    if (project_it == m_projects.end()) {
        return;
    }
    SceneInteractorProjectContext& context{project_it->second};
    context.wipe_tower_geometries[slicing_id.bed_instance_id] = wipe_tower;

    if (object_selection().is_selected(Domain::ElementRef{slicing_id})) {
        update_selection_bounding_box();
    }

    invoke_listeners<ISceneChangedListener>(
        [&](auto listener) { listener->on_wipe_tower_changed(slicing_id, wipe_tower); }
    );
}

void SceneInteractor::update_custom_gcode(
    const Domain::SlicingId slicing_id,
    const Domain::CustomGCode::Info& custom_gcode
)
{
    auto it{m_projects.find(slicing_id.project_id)};
    if (it == m_projects.end()) {
        return;
    }
    SceneInteractorProjectContext& project{it->second};

    Domain::BedInstance* bed_instance{
        project.project.find_bed_instance_by_id(slicing_id.bed_instance_id)
    };
    if (bed_instance == nullptr) {
        return;
    }
    bed_instance->custom_gcode = custom_gcode;

    const Domain::ConfigContainer* config_container{
        project.project.find_config_container_by_bed_instance_id(slicing_id.bed_instance_id)
    };
    if (config_container == nullptr) {
        return;
    }
    invoke_slicing_input_changed(Domain::BedRef{config_container->id().id, bed_instance->id().id});
}

void SceneInteractor::update_selection_bounding_box(
    const std::optional<SelectionExtents>& bounding_box
)
{
    const auto& project_it{m_projects.find(m_selected_project_id)};
    if (project_it == m_projects.end()) {
        return;
    }
    project_it->second.object_selection_bounding_box = std::nullopt;
    if (!project_it->second.object_selection.empty() && !bounding_box) {
        project_it->second.object_selection_bounding_box = Scene::get_selection_extents(
            m_selected_project_id,
            project_it->second.object_selection,
            *this,
            m_workbench
        );
    } else if (bounding_box) {
        project_it->second.object_selection_bounding_box = bounding_box;
    }
    invoke_listeners<ISceneSelectionChangedListener>(
        [&](auto* l)
        {
            l->on_scene_selection_bounding_box_updated(
                m_selected_project_id,
                project_it->second.object_selection
            );
        }
    );
}

const std::optional<SelectionExtents> SceneInteractor::selection_bounding_box() const
{
    const auto& project_it{m_projects.find(m_selected_project_id)};
    if (project_it == m_projects.end()) {
        return std::nullopt;
    }
    return project_it->second.object_selection_bounding_box;
}

const Print::WipeTowerGeometry* SceneInteractor::wipe_tower_geometry(
    std::size_t bed_instance_id
) const
{
    const auto& project_it{m_projects.find(m_selected_project_id)};
    if (project_it == m_projects.end()) {
        return nullptr;
    }
    const auto it{project_it->second.wipe_tower_geometries.find(bed_instance_id)};
    if (it == project_it->second.wipe_tower_geometries.end()) {
        return nullptr;
    }
    return &it->second;
}

void SceneInteractor::on_extruder_candidates_changed(
    std::vector<unsigned> extruder_candidates,
    const Domain::SlicingId slicing_id
)
{
    auto it{m_projects.find(slicing_id.project_id)};
    if (it == m_projects.end()) {
        return;
    }
    SceneInteractorProjectContext& project{it->second};

    const Domain::ConfigContainer* config_container{
        project.project.find_config_container_by_bed_instance_id(slicing_id.bed_instance_id)
    };
    if (config_container == nullptr) {
        return;
    }
    Domain::BedInstance* bed_instance{
        project.project.find_bed_instance_by_id(slicing_id.bed_instance_id)
    };
    if (bed_instance == nullptr) {
        return;
    }

    bed_instance->extruder_candidates = std::move(extruder_candidates);
    invoke_listeners<ISceneBedInstanceChangedListener>(
        [&](auto* l)
        {
            l->on_bed_instance_extruder_candidates_changed(
                slicing_id.project_id,
                Domain::BedRef{config_container->id().id, slicing_id.bed_instance_id},
                bed_instance->extruder_candidates
            );
        }
    );
}

BedTrackingChanges SceneInteractor::update_elements_bed_placement(const Domain::ElementRefs& elements, bool volume_mode)
{
    BedTrackingChanges changes;
    auto& proj          = m_projects.find(m_selected_project_id)->second;
    if (volume_mode) {
        std::set<size_t> object_ids;
        Domain::ElementRefs wipe_tower_refs;
        for (const auto& e : elements) {
            if (e.is_wipe_tower()) {
                wipe_tower_refs.push_back(e);
            } else {
                object_ids.insert(e.object_id);
            }
        }
        for (size_t obj_id : object_ids) {
            changes.append(
                m_bed_tracking.update_instances_bed_placement(proj.project, proj.project.find_object_by_id(obj_id)->instances)
            );
        }
        changes.append(
            m_bed_tracking.update_instances_bed_placement(proj.project, wipe_tower_refs)
        );
    } else {
        changes = m_bed_tracking.update_instances_bed_placement(proj.project, elements);
    }
    for (const auto& bed_ref : changes.updated_beds) {
        invoke_slicing_input_changed(bed_ref);
    }

    return changes;
}

void SceneInteractor::normalize_single_volume_object(Domain::ModelObject& object)
{
    if (object.volumes.size() != 1)
        return;
    
    // if the object constains only one volume
    // the volume transform is collapsed into the instance transforms

    Domain::ModelVolume* vol = object.volumes.front();
    Domain::Transform3d vol_trafo = vol->get_transformation().get_matrix();
    Domain::ElementRefs instance_refs;
    instance_refs.reserve(object.instances.size());
    Domain::ElementRefs volume_refs;
    volume_refs.reserve(object.instances.size());
    for (Domain::ModelInstance* inst : object.instances) {
        inst->set_transformation(Domain::Transformation(inst->get_transformation().get_matrix() * vol_trafo));
        instance_refs.emplace_back(object.id().id, inst->id().id);
        volume_refs.emplace_back(object.id().id, inst->id().id, vol->id().id);
    }
    vol->set_transformation(Domain::Transformation(Domain::Transform3d::Identity()));
    const auto changes = m_bed_tracking.update_instances_bed_placement(m_workbench.project(m_selected_project_id), instance_refs);

    invoke_listeners<ISceneChangedListener>(
        [&](ISceneChangedListener* l) {
            l->on_instance_transformed(m_selected_project_id, instance_refs, TransformState::Completed, changes);
            l->on_volume_transformed(m_selected_project_id, volume_refs, TransformState::Completed, changes);
        }
    );

    if (!changes.updated_instances.empty()) {
        invoke_listeners<ISceneChangedListener>(
            [&](ISceneChangedListener* l)
            {
                l->on_instances_last_bed_updated(
                    {changes.updated_instances.cbegin(), changes.updated_instances.cend()}
                );
            }
        );
    }
}

void SceneInteractor::invoke_slicing_input_changed(const Domain::BedRef& bed_instance)
{
    invoke_listeners<ISlicingInputChangedListener>(
        [&](auto listener) { listener->on_slicing_input_changed(bed_instance); }
    );
}

} // namespace Slic3r::Biz::Scene
