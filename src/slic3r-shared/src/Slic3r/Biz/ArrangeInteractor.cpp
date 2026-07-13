#include <span>
#include "Slic3r/Biz/ArrangeInteractor.hpp"
#include "Slic3r/Biz/Algorithms/ClipperUtils.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include "Slic3r/Biz/Arrange/ArrangeItem.hpp"
#include "Slic3r/Biz/Arrange/Arrange.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/Algorithms/ExPolygon.hpp"

#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Math.hpp"
#include "libslic3r/TriangleMeshSlicer.hpp" // project_mesh

namespace Slic3r::Biz {

using Algorithms::BoundingBox::merge;
using Algorithms::BoundingBox::to_2d;
using Algorithms::ClipperUtils::shrink;
using Algorithms::ClipperUtils::union_ex;
using Algorithms::ExPolygon::area;
using Algorithms::Point::to_2d;
using Algorithms::Scaling::scaled;
using Algorithms::Scaling::unscaled;
using Arrange::ArbitraryShape;
using Arrange::ArrangeItem;
using Arrange::ArrangeResult;
using Arrange::InputShape;
using Arrange::Settings;
using Arrange::StopCondition;
using Arrange::to_arrange_items;
using Biz::Arrange::Mode;
using Biz::JThread::StopToken;
using Domain::BedInstance;
using Domain::BedRef;
using Domain::BedRefs;
using Domain::BoundingBox2crd;
using Domain::BoundingBox2d;
using Domain::BoundingBox3d;
using Domain::ConfigContainer;
using Domain::ConfigItem;
using Domain::ConfigPack;
using Domain::ConfigPackFDM;
using Domain::ConstModelInstanceList;
using Domain::ElementRef;
using Domain::ElementRefs;
using Domain::ExPolygons;
using Domain::ModelInstance;
using Domain::ModelInstanceList;
using Domain::ModelObject;
using Domain::ModelVolumeType;
using Domain::Points;
using Domain::Polygon;
using Domain::Polygons;
using Domain::Project;
using Domain::SelectionId;
using Domain::Transform3d;
using Domain::Transformation;
using Domain::TriangleMesh;
using Domain::Vec2crd;
using Domain::Vec2d;
using Domain::Vec2ds;
using Domain::Vec3d;
using Domain::Workbench;
using Platform::PlatformServices;
using Platform::JobManager::JobManager;
using Platform::JobManager::ProgressTracker;
using Scene::BedInstances;
using Scene::BedSelection;
using Scene::get_selected_beds;

using Trafo  = Arrange::InstanceTransform2D;
using Trafos = Arrange::InstanceTransforms;

namespace BB = Biz::Algorithms::BoundingBox;

namespace {

BoundingBox3d instance_bounding_box(const ModelInstance& instance)
{
    Domain::BoundingBox3d result;
    const Transform3d inst_matrix{instance.get_transformation().get_matrix()};

    for (const Domain::ModelVolume* volume : instance.get_object()->volumes) {
        if (!volume->is_model_part()) {
            continue;
        }
        result = merge(
            result,
            Algorithms::BoundingBox::transformed(
                volume->mesh().bounding_box(),
                inst_matrix * volume->get_matrix()
            )
        );
    }

    return result;
}

constexpr double UnscaledCoordLimit = 1000.;

bool check_coord_bounds(const BoundingBox2d& bb)
{
    const Vec2d bb_size{Algorithms::BoundingBox::sizes(bb)};
    return std::abs(bb_size.x()) < UnscaledCoordLimit && std::abs(bb_size.y()) < UnscaledCoordLimit;
}

using TriangleMeshPtr = std::shared_ptr<const TriangleMesh>;

struct InstanceMesh
{
    TriangleMeshPtr mesh_ptr;
    Transform3d trafo;
    ModelVolumeType type;
};

struct InstanceMeshes
{
    Domain::ElementRef element_ref;
    std::vector<InstanceMesh> meshes;
};

struct CanceledException : public std::runtime_error
{
    CanceledException() : std::runtime_error{"canceled"} {}
};

std::optional<ArbitraryShape> extract_outline(const InstanceMeshes& meshes, const StopCondition stop_condition)
{
    ArbitraryShape result;

    const auto throw_on_cancel{[stop_condition]() {
        if (stop_condition()) {
            throw CanceledException{};
        }
    }};

    for (const InstanceMesh& mesh : meshes.meshes) {
        try {
            if (mesh.type != ModelVolumeType::MODEL_PART) {
                continue;
            }
            const Polygons vol_outline{project_mesh(mesh.mesh_ptr->its, mesh.trafo, stop_condition, 0.0)};
            result = union_ex(result, vol_outline);
        } catch (const CanceledException&) {
            return std::nullopt;
        }
    }

    return result;
}

Points get_bed_contour(const ArrangeBed& arrange_bed, const Settings& settings)
{
    const Domain::Bed& bed{arrange_bed.bed_instance.bed.get()};
    Points points;
    for (const Vec2d& point : bed.contour()) {
        points.push_back(scaled(point));
    }

    const int offset{
        static_cast<int>(settings.scaled_offset)
        - scaled(arrange_bed.offset)
        - scaled(0.1) // Add 0.1 mm safety offset from the bed boundary.
    };
    const Polygons offset_contour{Algorithms::ClipperUtils::offset({Polygon{points}}, offset)};

    if (offset_contour.size() != 1) {
        return points;
    }

    return offset_contour.front().points;
}

enum class ResetTranslation
{
    True,
    False
};

using MeshTranslation = std::variant<ResetTranslation, Vec3d>;

std::vector<InstanceMeshes> get_meshes(
    const ConstModelInstanceList& instances,
    const MeshTranslation translation
)
{
    std::vector<InstanceMeshes> result;
    result.reserve(instances.size());
    for (const ModelInstance* instance : instances) {
        if (!check_coord_bounds(to_2d(instance_bounding_box(*instance)))) {
            throw ArrangeFatalError{"Instance too large to be arranged!"};
        }

        std::vector<InstanceMesh> meshes;
        for (const Domain::ModelVolume* volume : instance->get_object()->volumes) {
            Transform3d instance_trafo{instance->get_matrix()};

            if (std::holds_alternative<ResetTranslation>(translation)) {
                const auto reset_translation{std::get<ResetTranslation>(translation)};
                if (reset_translation == ResetTranslation::True) {
                    instance_trafo.translation().x() = 0;
                    instance_trafo.translation().y() = 0;
                }
            } else {
                const auto translation_vector{std::get<Vec3d>(translation)};
                instance_trafo.translation() += translation_vector;
            }

            InstanceMesh mesh{
                .mesh_ptr = volume->mesh_ptr(),
                .trafo    = instance_trafo * volume->get_matrix(),
                .type     = volume->type(),
            };

            meshes.push_back(std::move(mesh));
        }

        const ElementRef instance_ref{instance->get_object()->id().id, instance->id().id, 0};

        result.push_back(
            InstanceMeshes{
                .element_ref = instance_ref,
                .meshes      = std::move(meshes),
            }
        );
    }
    return result;
}

std::optional<std::vector<InputShape>> get_arrange_input(
    const std::vector<InstanceMeshes>& meshes,
    const StopCondition stop_condition
)
{
    std::vector<InputShape> result;
    result.reserve(meshes.size());
    for (const InstanceMeshes& instance : meshes) {
        const std::optional<ArbitraryShape> outline{extract_outline(instance, stop_condition)};
        if (!outline) {
            return std::nullopt;
        }
        if (outline->empty()) {
            continue;
        }
        result.push_back({instance.element_ref, *outline});
    }

    std::ranges::stable_sort(result, [](const InputShape& a, const InputShape& b) {
        const double a_area{area(a.shape)};
        const double b_area{area(b.shape)};

        if (a_area <= 0 || b_area <= 0) {
            return false;
        }

        const double percentage_diff{std::abs(a_area - b_area) / std::min(a_area, b_area)};
        if (percentage_diff < 0.05) {
            // a < b == false && b < a == false => a == b and the order is preserved in stable sort.
            return false;
        }

        return b_area < a_area;
    });

    return result;
}

double get_max_brim(const ConstModelInstanceList& instances)
{
    double result{0.0};
    for (const ModelInstance* instance : instances) {
        const std::optional<ConfigItem> brim_width{
            instance->get_object()->object_settings.overrides.get("brim_width")
        };
        if (brim_width) {
            result = std::max(result, brim_width->get<double>());
        }
    }
    return result;
}

ArrangeBed get_arrange_bed(
    const SelectionId project_id,
    const BedRef& bed_ref,
    const double instances_brim,
    const Settings& settings,
    const Workbench& workbench
)
{
    const Project& project{workbench.project(project_id)};
    const ConfigContainer* config_container{project.find_config_container(bed_ref.config_container_id)};

    double brim_width{0.0};
    const ConfigPack& config{config_container->build_print_config()};
    if (std::holds_alternative<ConfigPackFDM>(config)) {
        const ConfigPackFDM& fdm_config{std::get<ConfigPackFDM>(config)};
        brim_width = fdm_config.print.items.opt("brim_width").get<double>();
        brim_width = std::max(brim_width, instances_brim);
    }

    const BedInstance& bed_instance{
        ASSERT_VAL(config_container)->find_bed_instance(bed_ref.instance_id)
    };
    return {bed_instance, std::max(brim_width, settings.unscaled_bed_offset)};
}

struct BedWithInstances
{
    BedRef bed_ref;
    std::vector<InstanceMeshes> arrangeable_instances;
    std::vector<InstanceMeshes> fixed_instances;
    ArrangeBed arrange_bed;
};

using ModelInstancesPerBed = std::vector<BedWithInstances>;

std::vector<BedRef> get_selected_beds(

    const SelectionId project_id,
    const Workbench& workbench,
    const Scene::SceneInteractor& scene_interactor
)
{
    const Project& project{workbench.project(project_id)};
    const BedSelection& selection{scene_interactor.bed_selection()};
    const ConfigContainer* config_container{
        project.find_config_container(selection.config_container_id())
    };
    if (config_container == nullptr) {
        return {};
    }

    std::vector<BedRef> result;
    for (const auto& bed_instance : config_container->bed_instances()) {
        const BedRef bed_ref{config_container->id().id, bed_instance->id().id};
        if (!selection.is_selected(bed_ref)) {
            continue;
        }

        result.push_back(bed_ref);
    }
    return result;
}

ArrangeItem wipe_tower_to_arrange_item(
    SelectionId project_id,
    const BedRef bed_ref,
    const Slicing::WipeTowerGeometry& wipe_tower,
    const Domain::ModelWipeTower& model_wipe_tower,
    const Settings& settings
)
{
    ArrangeItem result{
        InputShape{
            Domain::ElementRef{Domain::SlicingId{project_id, bed_ref.instance_id}},
            {wipe_tower.get_outline(model_wipe_tower)}
        },
        settings
    };
    result.is_wipe_tower = true;
    result.gravity_sink = settings.auxiliary_travel_anchor;
    return result;
}

struct WipeTowerForArrange
{
    ArrangeItem wipe_tower;
    bool is_fixed{false};
    Vec2crd bed_position{Vec2crd::Zero()};
};

using WipeTowerPerBed = std::map<BedRef, WipeTowerForArrange>;

WipeTowerPerBed get_wipe_towers_per_bed(
    const SelectionId project_id,
    const Settings& settings,
    const Workbench& workbench,
    const Scene::SceneInteractor& scene_interactor
)
{
    const Project& project{workbench.project(project_id)};
    const Scene::BedSelection& bed_selection{scene_interactor.bed_selection()};
    const ConfigContainer* config_container{
        project.find_config_container(bed_selection.config_container_id())
    };
    if (config_container == nullptr) {
        return {};
    }

    WipeTowerPerBed result;
    for (const auto& bed_instance : config_container->bed_instances()) {
        const BedRef bed_ref{config_container->id().id, bed_instance->id().id};
        if (!bed_selection.is_selected(bed_ref)) {
            continue;
        }
        const Slicing::WipeTowerGeometry* wipe_tower_geometry{
            scene_interactor.wipe_tower_geometry(bed_ref.instance_id)
        };
        if (wipe_tower_geometry == nullptr) {
            continue;
        }

        result.insert(
            {bed_ref,
             {wipe_tower_to_arrange_item(
                  project_id,
                  bed_ref,
                  *wipe_tower_geometry,
                  bed_instance->wipe_tower,
                  settings
              ),
              false,
              scaled(bed_instance->wipe_tower.position)}}
        );
    }

    return result;
}

using IsArrangeablePredicate = std::function<bool(const ModelInstance*)>;

BedWithInstances get_bed_with_instances(
    const SelectionId project_id,
    const BedRef& bed_ref,
    const BedInstance& bed_instance,
    const Settings& settings,
    const Workbench& workbench,
    const IsArrangeablePredicate& is_arrangeable
)
{
    ModelInstanceList model_instances{bed_instance.model_instances};
    std::erase_if(model_instances, [](const ModelInstance* inst) { return !inst->printable; });

    ConstModelInstanceList arrangeable_model_intances;
    ConstModelInstanceList fixed_model_intances;

    for (const ModelInstance* instance : model_instances) {
        if (is_arrangeable(instance)) {
            arrangeable_model_intances.push_back(instance);
        } else {
            fixed_model_intances.push_back(instance);
        }
    }

    ArrangeBed arrange_bed{get_arrange_bed(
        project_id,
        bed_ref,
        get_max_brim(arrangeable_model_intances),
        settings,
        workbench
    )};

    const Vec3d bed_translation{arrange_bed.bed_instance.transformation.get_matrix().translation()};
    return {
        bed_ref,
        get_meshes(arrangeable_model_intances, ResetTranslation::True),
        get_meshes(fixed_model_intances, -bed_translation),
        std::move(arrange_bed)
    };
}

ModelInstancesPerBed get_model_instances_per_bed(
    const SelectionId project_id,
    const BedSelection& selection,
    const Settings& settings,
    const Workbench& workbench,
    const IsArrangeablePredicate& is_arrangeable
)
{
    ModelInstancesPerBed result;

    const Project& project{workbench.project(project_id)};
    const ConfigContainer* config_container{
        project.find_config_container(selection.config_container_id())
    };
    if (config_container == nullptr) {
        return {};
    }

    for (const auto& bed_instance : config_container->bed_instances()) {
        const BedRef bed_ref{config_container->id().id, bed_instance->id().id};
        if (!selection.is_selected(bed_ref)) {
            continue;
        }
        result.push_back(get_bed_with_instances(
            project_id,
            bed_ref,
            *bed_instance,
            settings,
            workbench,
            is_arrangeable
        ));
    }

    return result;
}

ModelInstancesPerBed get_model_instances_per_bed(
    const SelectionId project_id,
    const BedRef& bed_ref,
    const Settings& settings,
    const Workbench& workbench,
    const IsArrangeablePredicate& is_arrangeable
)
{
    const Project& project{workbench.project(project_id)};
    if (project.find_config_container(bed_ref.config_container_id) == nullptr) {
        return {};
    }

    const BedInstance* bed_instance{project.find_bed_instance_by_id(bed_ref.instance_id)};
    if (bed_instance == nullptr) {
        return {};
    }

    BedWithInstances result = get_bed_with_instances(
        project_id,
        bed_ref,
        *bed_instance,
        settings,
        workbench,
        is_arrangeable
    );

    // Arrangeable instances may not be tracked on the target bed (e.g. cross-bed
    // paste where clones remain at the source bed position). Find them directly
    // in the model so arrangement can place them on the target bed.
    std::set<size_t> instances_on_target_bed;
    for (const InstanceMeshes& meshes : result.arrangeable_instances) {
        instances_on_target_bed.insert(meshes.element_ref.instance_id);
    }

    ConstModelInstanceList untracked_instances;
    for (const ModelObject* model_object : project.model().objects) {
        for (const ModelInstance* model_instance : model_object->instances) {
            if (is_arrangeable(model_instance)
                && model_instance->printable
                && !instances_on_target_bed.contains(model_instance->id().id))
            {
                untracked_instances.push_back(model_instance);
            }
        }
    }

    if (!untracked_instances.empty()) {
        std::vector<InstanceMeshes> meshes =
            get_meshes(untracked_instances, ResetTranslation::True);
        result.arrangeable_instances.insert(
            result.arrangeable_instances.end(),
            std::make_move_iterator(meshes.begin()),
            std::make_move_iterator(meshes.end())
        );
    }

    return {std::move(result)};
}

void offset_trafos(Trafos& trafos, const Vec2d& offset)
{
    Trafos result;
    result.reserve(trafos.size());
    for (Trafo& trafo : trafos) {
        trafo.absolute_offset += offset;
    }
}

} // namespace

ArrangeInteractor::ArrangeInteractor(Scene::SceneInteractor& scene_interactor, const Workbench& workbench) :
    m_scene_interactor{scene_interactor},
    m_workbench{workbench}
{}

ConstModelInstanceList ArrangeInteractor::get_model_instances(
    const SelectionId project_id,
    const Scene::BedSelection& selection,
    const bool include_unplaced
) const
{
    ConstModelInstanceList result;
    const BedInstances beds{get_selected_beds(project_id, selection, m_workbench)};
    for (const auto& bed : beds) {
        const ModelInstanceList& instances{bed.get().model_instances};
        result.insert(result.end(), instances.begin(), instances.end());
    }

    if (include_unplaced) {
        const ModelInstanceList& instances{m_scene_interactor.unplaced_model_instances(project_id)};
        result.insert(result.end(), instances.begin(), instances.end());
    }

    std::erase_if(result, [](const ModelInstance* inst) {return !inst->printable; });

    return result;
}

double ArrangeInteractor::apply_arrange_result(
    const BedInstances& bed_instances,
    const double scaled_offset,
    const Packs& packs,
    const double initial_offset,
    ElementRefs* not_arranged
)
{
    const std::size_t existing_count{std::min(bed_instances.size(), packs.size())};

    std::size_t bed_index{0};
    for (Pack pack : std::span{packs}.subspan(0, existing_count)) {
        const BedInstance& bed_instance{bed_instances.at(bed_index).get()};
        Transformation bed_trafo{bed_instance.transformation};
        const Vec2d bed_offset{to_2d(bed_trafo.get_offset())};
        offset_trafos(pack.trafos, bed_offset);
        m_scene_interactor.transform_instances(pack.trafos);
        ++bed_index;
    }

    if (bed_instances.empty()) {
        return initial_offset;
    }

    double offset{initial_offset};
    for (Pack pack : std::span{packs}.subspan(existing_count)) {
        offset -= BB::sizes(pack.bounding_box).x();
        offset -= unscaled(static_cast<int>(scaled_offset));
        offset_trafos(pack.trafos, Vec2d{offset, 0.0});
        m_scene_interactor.transform_instances(pack.trafos);
        for (auto trafo : pack.trafos) {
            ASSERT(not_arranged);
            not_arranged->push_back(trafo.instance_ref);
        }
    }

    return offset;
}

double ArrangeInteractor::apply_arrange_result(
    const SelectionId project_id,
    const BedSelection& selection,
    const OverflowMode& overflow_mode,
    const double scaled_offset,
    const Packs& packs,
    const double initial_offset,
    ElementRefs* not_arranged
)
{
    BedInstances bed_instances{get_selected_beds(project_id, selection, m_workbench)};

    if (overflow_mode == OverflowMode::AddBeds) {
        const std::size_t existing_count{std::min(bed_instances.size(), packs.size())};
        const std::size_t remaining_count{packs.size() - existing_count};
        for (std::size_t i{0}; i < remaining_count; ++i) {
            const SelectionId config_container_id{selection.config_container_id()};
            const BedInstance& bed_instance{
                m_scene_interactor.add_bed_instance(config_container_id)
            };
            bed_instances.emplace_back(bed_instance);
        }
    } else {
        ASSERT(overflow_mode == OverflowMode::MoveNextToFirstBed);
    }

    return apply_arrange_result(bed_instances, scaled_offset, packs, initial_offset, not_arranged);
}

double ArrangeInteractor::apply_arrange_result(
    const SelectionId project_id,
    const BedRef& bed_ref,
    const double scaled_offset,
    const Packs& packs,
    const double initial_offset,
    ElementRefs* not_arranged
)
{
    BedInstances bed_instances;
    const BedInstance* bed_instance{
        m_workbench.project(project_id).find_bed_instance_by_id(bed_ref.instance_id)
    };
    if (bed_instance != nullptr) {
        bed_instances.emplace_back(*bed_instance);
    }

    return apply_arrange_result(bed_instances, scaled_offset, packs, initial_offset, not_arranged);
}

namespace {

struct PackingResult
{
    Packs packs;
    std::vector<ArrangeItem> unpacked;
};

std::optional<PackingResult> pack_to_bed(
    std::vector<ArrangeItem>& to_pack,
    std::vector<ArrangeItem>& extra,
    const std::vector<ArrangeItem>& fixed,
    const ArrangeBed arrange_bed,
    const StopCondition stop_condition,
    Settings settings,
    const std::vector<BedRef>& available_beds,
    const WipeTowerPerBed& wipe_tower_per_bed
)
{
    Packs packs;
    const std::size_t limit{1'000};

    std::vector<ArrangeItem> extra_to_pack{extra};

    for (std::size_t index{}; index < limit; ++index) {
        ASSERT(index < limit - 1, "Infinite arrange loop!");

        std::optional<WipeTowerForArrange> wipe_tower;
        if (index < available_beds.size()) {
            const auto it{wipe_tower_per_bed.find(available_beds[index])};
            if (it != wipe_tower_per_bed.end()) {
                wipe_tower = it->second;
            }
        }

        const Points bed_contour{get_bed_contour(arrange_bed, settings)};

        std::vector<ArrangeItem> to_pack_with_tower{};
        std::vector<ArrangeItem> fixed_with_tower{};
        if (wipe_tower) {
            if (wipe_tower->is_fixed) {
                fixed_with_tower.push_back(wipe_tower->wipe_tower);
            } else {
                to_pack_with_tower.push_back(wipe_tower->wipe_tower);
                if (wipe_tower->wipe_tower.gravity_sink) {
                    settings.strategy = Arrange::Strategy::Gravity;
                }
            }
        }
        to_pack_with_tower.insert(to_pack_with_tower.end(), to_pack.begin(), to_pack.end());
        fixed_with_tower.insert(fixed_with_tower.end(), fixed.begin(), fixed.end());

        std::optional<ArrangeResult> result{Arrange::arrange(
            bed_contour,
            to_pack_with_tower,
            fixed_with_tower,
            settings,
            stop_condition
        )};
        if (!result) {
            return std::nullopt;
        }

        std::vector<ArrangeItem> fixed_for_extra{result->packed};
        fixed_for_extra.insert(fixed_for_extra.end(), fixed_with_tower.begin(), fixed_with_tower.end());
        std::optional<ArrangeResult> extra_result{
            Arrange::arrange(bed_contour, extra_to_pack, fixed_for_extra, settings, stop_condition)
        };
        if (!extra_result) {
            return std::nullopt;
        }
        extra_to_pack = extra_result->not_packed;

        std::vector<ArrangeItem> packed{result->packed};
        packed.insert(packed.end(), extra_result->packed.begin(), extra_result->packed.end());

        if (packed.empty()) {
            break;
        }

        Pack pack;
        pack.bounding_box = BB::unscaled<double>(BB::construct(bed_contour));
        for (ArrangeItem& item : packed) {
            const Vec2d item_offset{unscaled<double>(item.get_translation())};

            pack.trafos.push_back(
                Trafo{
                    .instance_ref    = item.get_element_ref(),
                    .absolute_offset = item_offset,
                    .rotation_delta  = item.get_rotation()
                }
            );
        }
        packs.push_back(std::move(pack));

        to_pack = result->not_packed;
    }
    PackingResult result{packs, extra_to_pack};
    result.unpacked.insert(result.unpacked.end(), to_pack.begin(), to_pack.end());
    return result;
}

struct ArrangeGlobalResult {
    Packs packs;
    std::vector<ArrangeItem> failed;
};

std::optional<ArrangeGlobalResult> arrange_global(
    StopToken stop_token,
    ProgressTracker progress_tracker,
    const Settings settings,
    const ArrangeBed arrange_bed,
    const std::vector<InstanceMeshes> instance_meshes,
    const std::vector<BedRef> available_beds,
    const WipeTowerPerBed wipe_tower_per_bed
)
{
    const auto stop_condition{[=]() {
        return stop_token.stop_requested();
    }};

    const std::optional<std::vector<InputShape>> arrange_input{
        get_arrange_input(instance_meshes, stop_condition)
    };
    if (!arrange_input) {
        return std::nullopt;
    }

    std::vector<ArrangeItem> to_pack{to_arrange_items(*arrange_input, settings)};
    std::vector<ArrangeItem> extra;
    if (auto result{pack_to_bed(
            to_pack,
            extra,
            {},
            arrange_bed,
            stop_condition,
            settings,
            available_beds,
            wipe_tower_per_bed
        )})
    {
        return ArrangeGlobalResult{std::move(result->packs), std::move(result->unpacked)};
    }
    return std::nullopt;
}

struct ArrangeLocalResult {
    std::vector<std::pair<BedRef, Packs>> packs;
    std::vector<ArrangeItem> failed;
};

std::optional<ArrangeLocalResult> arrange_local(
    StopToken stop_token,
    ProgressTracker progress_tracker,
    const Settings settings,
    const ModelInstancesPerBed model_instances_per_bed,
    const std::vector<InstanceMeshes> extra_instances,
    const WipeTowerPerBed wipe_tower_per_bed
)
{
    const auto stop_condition{[=]() {
        return stop_token.stop_requested();
    }};

    std::vector<std::pair<BedRef, Packs>> packs;

    const std::optional<std::vector<InputShape>> extra_input{
        get_arrange_input(extra_instances, stop_condition)
    };
    if (!extra_input) {
        return std::nullopt;
    }
    std::vector<ArrangeItem> extra{to_arrange_items(*extra_input, settings)};

    for (const BedWithInstances& bed_with_instances : model_instances_per_bed) {
        const std::optional<std::vector<InputShape>> arrangeable_input{
            get_arrange_input(bed_with_instances.arrangeable_instances, stop_condition)
        };
        if (!arrangeable_input) {
            return std::nullopt;
        }
        const std::optional<std::vector<InputShape>> fixed_input{
            get_arrange_input(bed_with_instances.fixed_instances, stop_condition)
        };
        if (!fixed_input) {
            return std::nullopt;
        }

        std::vector<ArrangeItem> arrangeable{to_arrange_items(*arrangeable_input, settings)};
        const std::vector<ArrangeItem> fixed{to_arrange_items(*fixed_input, settings)};
        if (auto packing_result{pack_to_bed(
                arrangeable,
                extra,
                fixed,
                bed_with_instances.arrange_bed,
                stop_condition,
                settings,
                {bed_with_instances.bed_ref},
                wipe_tower_per_bed
            )})
        {
            packs.push_back({bed_with_instances.bed_ref, std::move(packing_result->packs)});
            extra = packing_result->unpacked;
        } else {
            return std::nullopt;
        }
    }

    return ArrangeLocalResult{
        packs,
        extra
    };
}

std::set<SelectionId> get_extra_selected_instances(
    const Scene::ObjectSelection& object_selection,
    const ModelInstancesPerBed& model_instances_per_bed
)
{
    std::set<SelectionId> result;
    for (const ElementRef& element : object_selection.elements) {
        if (element.has_instance()) {
            result.insert(element.instance_id);
        }
    }
    for (const BedWithInstances& bed_with_instances : model_instances_per_bed) {
        for (const InstanceMeshes& instance_meshes : bed_with_instances.arrangeable_instances) {
            ASSERT(instance_meshes.element_ref.has_instance());
            result.erase(instance_meshes.element_ref.instance_id);
        }
    }
    return result;
}

ConstModelInstanceList get_instances(
    const SelectionId project_id,
    const std::set<SelectionId>& instance_ids,
    const Workbench& workbench
)
{
    ConstModelInstanceList result;

    const Domain::Model& model{workbench.project(project_id).model()};
    for (const ModelObject* object : model.objects) {
        for (const ModelInstance* instance : object->instances) {
            if (instance_ids.contains(instance->id().id)) {
                result.push_back(instance);
            }
        }
    }
    std::erase_if(result, [](const ModelInstance* inst) { return !inst->printable; });
    return result;
}

} // namespace

void ArrangeInteractor::arrange(
    const Domain::SelectionId project_id,
    const Settings& settings,
    std::function<void()> on_finished
)
{
    if (project_id == Domain::INVALID_ID) {
        if (on_finished) {
            on_finished();
        }

        return;
    }

    const double unplaced_offset{-20.0};

    const std::vector<BedRef> selected_beds{
        get_selected_beds(project_id, m_workbench, m_scene_interactor)
    };
    WipeTowerPerBed wipe_tower_per_bed{get_wipe_towers_per_bed(
        project_id,
        settings,
        m_workbench,
        m_scene_interactor
    )};


    JobManager& job_manager{PlatformServices::instance().job_manager()};
    try {
        if (settings.mode == Mode::Global) {
            const BedRef last_selected_bed{m_scene_interactor.bed_selection().last_selected_bed()};
            const ConstModelInstanceList instances{
                get_model_instances(project_id, m_scene_interactor.bed_selection(), true)
            };
            job_manager
                .create_job(
                    "arrange",
                    arrange_global,
                    settings,
                    get_arrange_bed(
                        project_id,
                        last_selected_bed,
                        get_max_brim(instances),
                        settings,
                        m_workbench
                    ),
                    get_meshes(instances, ResetTranslation::True),
                    selected_beds,
                    wipe_tower_per_bed
                )
                .on_result(
                    [this, project_id, settings, unplaced_offset, on_finished](const std::optional<ArrangeGlobalResult>& result)
                    {
                        if (!result) {
                            if (on_finished) {
                                on_finished();
                            }

                            return;
                        }

                        ElementRefs not_arranged;
                        for (const ArrangeItem& arrange_item : result->failed) {
                            not_arranged.push_back(arrange_item.get_element_ref());
                        }
                        apply_arrange_result(
                            project_id,
                            m_scene_interactor.bed_selection(),
                            OverflowMode::AddBeds,
                            settings.scaled_offset,
                            result->packs,
                            unplaced_offset,
                            &not_arranged
                        );
                        on_finished();
                        if (!not_arranged.empty()) {
                            invoke_listeners<IArrangeEventsListener>(
                                [&](auto* listener)
                                { listener->on_elements_not_arranged(project_id, not_arranged); }
                            );
                        }
                    }
                )
                .start();
        } else if (settings.mode == Mode::Local) {
            const Scene::ObjectSelection& object_selection{m_scene_interactor.object_selection()};
            const IsArrangeablePredicate is_arrangeable =
                [&object_selection](const ModelInstance* instance) -> bool
            {
                if (object_selection.empty()) {
                    return true;
                }

                const ElementRef ref{instance->get_object()->id().id, instance->id().id};
                return object_selection.is_selected(ref);
            };

            const ModelInstancesPerBed model_instances_per_bed{get_model_instances_per_bed(
                project_id,
                m_scene_interactor.bed_selection(),
                settings,
                m_workbench,
                is_arrangeable
            )};

            std::vector<InstanceMeshes> selected_extra_model_instances;
            if (!object_selection.empty()) {
                const std::set<SelectionId> instance_ids{
                    get_extra_selected_instances(object_selection, model_instances_per_bed)
                };
                const ConstModelInstanceList instances{
                    get_instances(project_id, instance_ids, m_workbench)
                };
                selected_extra_model_instances = get_meshes(instances, ResetTranslation::True);

                for (auto& [bed_ref, wipe_tower] : wipe_tower_per_bed) {
                    if (!object_selection.is_selected(wipe_tower.wipe_tower.get_element_ref())) {
                        wipe_tower.is_fixed = true;
                        wipe_tower.wipe_tower.set_translation(wipe_tower.bed_position);
                    }
                }
            }

            job_manager
                .create_job(
                    "arrange",
                    arrange_local,
                    settings,
                    model_instances_per_bed,
                    selected_extra_model_instances,
                    wipe_tower_per_bed
                )
                .on_result(
                    [this, project_id, settings, unplaced_offset, on_finished](
                        const std::optional<ArrangeLocalResult>& result
                    )
                    {
                        if (!result) {
                            if (on_finished) {
                                on_finished();
                            }

                            return;
                        }

                        ElementRefs not_arranged;
                        for (const ArrangeItem& arrange_item : result->failed) {
                            not_arranged.push_back(arrange_item.get_element_ref());
                        }
                        double offset{unplaced_offset};

                        for (const auto& [bed_ref, packs] : result->packs) {
                            m_scene_interactor.bed_selection().select_one(
                                bed_ref,
                                Scene::CameraActionOnBedSelection::CenterOnBed
                            );
                            offset = apply_arrange_result(
                                project_id,
                                bed_ref,
                                settings.scaled_offset,
                                packs,
                                offset,
                                &not_arranged
                            );
                        }
                        on_finished();
                        if (!not_arranged.empty()) {
                            invoke_listeners<IArrangeEventsListener>(
                                [&](auto* listener)
                                { listener->on_elements_not_arranged(project_id, not_arranged); }
                            );
                        }
                    }
                )
                .start();
        } else {
            PANIC("Unknown arrange mode!");
        }

    } catch (const ArrangeFatalError&) {
        invoke_listeners<IArrangeEventsListener>([&](auto* listener)
                                                 { listener->on_fatal_arrange_error(project_id); });

        if (on_finished) {
            on_finished();
        }
    }
}

void ArrangeInteractor::partial_arrange(
    const SelectionId project_id,
    const std::set<size_t>& arrangeable_instance_ids,
    const BedRef& target_bed,
    const Settings& settings,
    PartialArrangeCallback on_completed
)
{
    if (project_id == Domain::INVALID_ID || arrangeable_instance_ids.empty()) {
        if (on_completed) {
            on_completed({});
        }

        return;
    }

    const IsArrangeablePredicate is_arrangeable =
        [&arrangeable_instance_ids](const ModelInstance* instance)
    { return arrangeable_instance_ids.contains(instance->id().id); };

    const double unplaced_offset{-20.0};
    const ModelInstancesPerBed model_instances_per_bed{
        get_model_instances_per_bed(project_id, target_bed, settings, m_workbench, is_arrangeable)
    };

    if (model_instances_per_bed.empty()) {
        if (on_completed) {
            on_completed({});
        }

        return;
    }

    WipeTowerPerBed wipe_tower_per_bed{
        get_wipe_towers_per_bed(project_id, settings, m_workbench, m_scene_interactor)
    };
    for (auto& [bed_ref, wipe_tower] : wipe_tower_per_bed) {
        wipe_tower.is_fixed = true;
        wipe_tower.wipe_tower.set_translation(wipe_tower.bed_position);
    }

    JobManager& job_manager{PlatformServices::instance().job_manager()};
    try {
        job_manager
            .create_job(
                "partial_arrange",
                arrange_local,
                settings,
                model_instances_per_bed,
                std::vector<InstanceMeshes>{},
                wipe_tower_per_bed
            )
            .on_result(
                [this,
                 project_id,
                 settings,
                 unplaced_offset,
                 on_completed =
                     std::move(on_completed)](const std::optional<ArrangeLocalResult>& result)
                {
                    if (!result) {
                        return;
                    }

                    ElementRefs not_arranged;
                    for (const ArrangeItem& arrange_item : result->failed) {
                        not_arranged.push_back(arrange_item.get_element_ref());
                    }

                    double offset{unplaced_offset};
                    for (const auto& [bed_ref, packs] : result->packs) {
                        offset = apply_arrange_result(
                            project_id,
                            bed_ref,
                            settings.scaled_offset,
                            packs,
                            offset,
                            &not_arranged
                        );
                    }

                    if (on_completed) {
                        on_completed(not_arranged);
                    }
                }
            )
            .start();
    } catch (const ArrangeFatalError&) {
        invoke_listeners<IArrangeEventsListener>([&](auto* listener)
                                                 { listener->on_fatal_arrange_error(project_id); });
    }
}

void ArrangeInteractor::arrange_added_instances(
    const SelectionId project_id,
    const ElementRefs& added_instances,
    const BedRef& target_bed,
    const UndoSnapshotType snapshot_type
)
{
    std::set<size_t> instance_ids;
    for (const ElementRef& ref : added_instances) {
        instance_ids.insert(ref.instance_id);
    }

    bool is_queue_processing_running = false;
    {
        std::lock_guard lock(m_added_arrange_mutex);
        is_queue_processing_running = !m_added_arrange_queue.empty();
        m_added_arrange_queue.push(
            {project_id, std::move(instance_ids), target_bed, snapshot_type}
        );
    }

    if (!is_queue_processing_running) {
        this->process_added_arrange_queue();
    }
}

void ArrangeInteractor::process_added_arrange_queue()
{
    const constexpr Settings ARRANGE_SETTINGS{
        .strategy        = Arrange::Strategy::Gravity,
        .scaled_offset   = Algorithms::Scaling::scaled(3.0),
        .allow_rotations = false
    };

    PendingArrange pending_arrange;
    {
        std::lock_guard lock(m_added_arrange_mutex);
        if (m_added_arrange_queue.empty()) {
            return;
        }

        pending_arrange = m_added_arrange_queue.front();
    }

    this->partial_arrange(
        pending_arrange.project_id,
        pending_arrange.instance_ids,
        pending_arrange.target_bed,
        ARRANGE_SETTINGS,
        [this,
         project_id    = pending_arrange.project_id,
         target_bed    = pending_arrange.target_bed,
         snapshot_type = pending_arrange.snapshot_type,
         attempted_ids = pending_arrange.instance_ids](const ElementRefs& not_arranged)
        {
            // If nothing was placed and the bed had no other instances, the bed was empty and the
            // instances are too big for it, so do not move them onto a new empty bed.
            bool instances_cannot_fit_on_bed = false;
            if (!not_arranged.empty() && not_arranged.size() == attempted_ids.size()) {
                const Project& project = m_workbench.project(project_id);
                const BedInstance* bed = project.find_bed_instance_by_id(target_bed.instance_id);

                bool bed_has_other_instances = false;
                if (bed != nullptr) {
                    for (const ModelInstance* inst : bed->model_instances) {
                        // Skip our own instances, because they are on the bed too.
                        // Any other instance means the bed wasn't empty.
                        if (!attempted_ids.contains(inst->id().id)) {
                            bed_has_other_instances = true;
                            break;
                        }
                    }
                }

                instances_cannot_fit_on_bed = !bed_has_other_instances;
            }

            if (!not_arranged.empty() && !instances_cannot_fit_on_bed) {
                const Project& project                = m_workbench.project(project_id);
                const SelectionId config_container_id = target_bed.config_container_id;

                const Domain::ConfigContainer* config_container =
                    project.find_config_container(config_container_id);
                ASSERT(config_container != nullptr);

                const BedRef last_bed_in_container{
                    config_container_id,
                    config_container->bed_instances().back()->id().id
                };

                BedRef next_bed = last_bed_in_container;
                if (target_bed == last_bed_in_container) {
                    // Create a new bed and arrange objects there.
                    const BedInstance& new_bed =
                        m_scene_interactor.add_bed_instance(config_container_id);
                    next_bed = BedRef{config_container_id, new_bed.id().id};
                    m_scene_interactor.bed_selection().toggle(next_bed);
                }

                std::set<size_t> not_arranged_ids;
                for (const ElementRef& ref : not_arranged) {
                    not_arranged_ids.insert(ref.instance_id);
                }

                this->move_instances_to_bed(project_id, not_arranged, next_bed);

                {
                    std::lock_guard lock(m_added_arrange_mutex);
                    m_added_arrange_queue.pop();
                    m_added_arrange_queue.push(
                        {project_id, std::move(not_arranged_ids), next_bed, snapshot_type}
                    );
                }
            } else {
                if (!not_arranged.empty()) {
                    // These instances cannot fit any bed.
                    invoke_listeners<IArrangeEventsListener>(
                        [&](auto* listener)
                        { listener->on_elements_not_arranged(project_id, not_arranged); }
                    );
                }

                {
                    std::lock_guard lock(m_added_arrange_mutex);
                    m_added_arrange_queue.pop();
                }

                // Take the undo snapshot only once the whole arrange operation has finished.
                m_scene_interactor.undo_provider().take_snapshot(snapshot_type);
            }

            this->process_added_arrange_queue();
        }
    );
}

void ArrangeInteractor::move_instances_to_bed(
    const SelectionId project_id,
    const ElementRefs& instances,
    const BedRef& bed_ref
)
{
    const BedInstance* bed_instance =
        m_workbench.project(project_id).find_bed_instance_by_id(bed_ref.instance_id);
    if (bed_instance == nullptr) {
        return;
    }

    const Vec2d bed_offset =
        Algorithms::Point::to_2d(Transformation{bed_instance->transformation}.get_offset());
    const Vec2d bed_center = bed_offset + bed_instance->bed.get().center();

    Arrange::InstanceTransforms trafos;
    for (const ElementRef& ref : instances) {
        trafos.push_back(
            {.instance_ref = ref, .absolute_offset = bed_center, .rotation_delta = 0.0}
        );
    }

    m_scene_interactor.transform_instances(trafos);
}

} // namespace Slic3r::Biz
