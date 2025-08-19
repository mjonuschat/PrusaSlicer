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
#include "libslic3r/TriangleMeshSlicer.hpp" // project_mesh

namespace Slic3r::Biz {

using Algorithms::BoundingBox::merge;
using Algorithms::BoundingBox::to_2d;
using Algorithms::ClipperUtils::diff_ex;
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
using Scene::BedSelection;
using Scene::BedInstances;
using Scene::get_selected_beds;

using Trafo  = Scene::SceneInteractor::InstanceTransform2D;
using Trafos = Scene::SceneInteractor::InstanceTransforms;

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
        result = merge(result, volume->mesh().bounding_box());
    }

    return result;
}

constexpr double UnscaledCoordLimit = 1000.;

bool check_coord_bounds(const BoundingBox2d& bb)
{
    return std::abs(bb.min.x()) < UnscaledCoordLimit
        && std::abs(bb.min.y()) < UnscaledCoordLimit
        && std::abs(bb.max.x()) < UnscaledCoordLimit
        && std::abs(bb.max.y()) < UnscaledCoordLimit;
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
            const Polygons vol_outline{project_mesh(mesh.mesh_ptr->its, mesh.trafo, stop_condition)};
            switch (mesh.type) {
            case ModelVolumeType::MODEL_PART:
                result = union_ex(result, vol_outline);
                break;
            case ModelVolumeType::NEGATIVE_VOLUME:
                result = diff_ex(result, vol_outline);
                break;
            default:;
            }
        } catch (const CanceledException&) {
            return std::nullopt;
        }
    }

    return result;
}

Points get_bed_contour(const ArrangeBed& arrange_bed)
{
    const Domain::Bed& bed{arrange_bed.bed_instance.bed.get()};
    Points points;
    for (const Vec2d& point : bed.contour()) {
        points.push_back(scaled(point));
    }

    if (arrange_bed.offset <= 0) {
        return points;
    }

    const Polygons offset_contour{shrink({Polygon{points}}, scaled(arrange_bed.offset))};

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
                    instance_trafo.translation() = Vec3d::Zero();
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
    const ConfigPack& config{config_container->print_config()};
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

ModelInstancesPerBed get_model_instances_per_bed(
    const SelectionId project_id,
    const BedSelection& selection,
    const Settings& settings,
    const Workbench& workbench,
    const std::set<SelectionId>& selected_instances
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
        const ModelInstanceList model_instances{bed_instance->model_instances};

        ConstModelInstanceList arrangeable_model_intances;
        ConstModelInstanceList fixed_model_intances;

        if (selected_instances.empty()) {
            arrangeable_model_intances.insert(
                arrangeable_model_intances.end(),
                model_instances.begin(),
                model_instances.end()
            );
        } else {
            for (const ModelInstance* instance : model_instances) {
                if (selected_instances.contains(instance->id().id)) {
                    arrangeable_model_intances.push_back(instance);
                } else {
                    fixed_model_intances.push_back(instance);
                }
            }
        }

        ArrangeBed arrange_bed{
            get_arrange_bed(project_id, bed_ref, get_max_brim(arrangeable_model_intances), settings, workbench)
        };

        const Vec3d bed_translation{arrange_bed.bed_instance.transformation.get_matrix().translation()};
        result.push_back(
            {bed_ref,
             get_meshes(arrangeable_model_intances, ResetTranslation::True),
             get_meshes(fixed_model_intances, -bed_translation),
             std::move(arrange_bed)}
        );
    }
    return result;
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

    return result;
}

double ArrangeInteractor::apply_arrange_result(
    const SelectionId project_id,
    const BedSelection& selection,
    const OverflowMode& overflow_mode,
    const double scaled_offset,
    const Packs& packs,
    const double initial_offset
)
{
    BedInstances bed_instances{get_selected_beds(project_id, selection, m_workbench)};

    std::size_t existing_count{std::min(bed_instances.size(), packs.size())};
    std::size_t remaining_count{packs.size() - existing_count};

    if (overflow_mode == OverflowMode::AddBeds) {
        for (std::size_t _{}; _ < remaining_count; ++_) {
            const SelectionId config_container_id{selection.config_container_id()};
            const BedInstance& bed_instance{m_scene_interactor.add_bed_instance(config_container_id)};
            bed_instances.push_back(bed_instance);
        }
        existing_count += remaining_count;
        remaining_count = 0;
    } else {
        ASSERT(overflow_mode == OverflowMode::MoveNextToFirstBed);
    }

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
    }

    return offset;
}

namespace {

struct PackingResult
{
    Packs packs;
    std::vector<ArrangeItem> unpacked_extra;
};

std::optional<PackingResult> pack_to_bed(
    std::vector<ArrangeItem>& to_pack,
    std::vector<ArrangeItem>& extra,
    const std::vector<ArrangeItem>& fixed,
    const ArrangeBed arrange_bed,
    const StopCondition stop_condition,
    const Settings settings
)
{
    Packs packs;
    const std::size_t limit{1'000};

    std::vector<ArrangeItem> extra_to_pack{extra};

    for (std::size_t count{}; count < limit; ++count) {
        ASSERT(count < limit - 1, "Infinite arrange loop!");

        const Points bed_contour{get_bed_contour(arrange_bed)};

        std::optional<ArrangeResult> result{
            Arrange::arrange(bed_contour, to_pack, fixed, settings, stop_condition)
        };
        if (!result) {
            return std::nullopt;
        }

        std::vector<ArrangeItem> fixed_for_extra{result->packed};
        fixed_for_extra.insert(fixed_for_extra.end(), fixed.begin(), fixed.end());
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
    return PackingResult{packs, extra_to_pack};
}

std::optional<Packs> arrange_global(
    StopToken stop_token,
    ProgressTracker progress_tracker,
    const Settings settings,
    const ArrangeBed arrange_bed,
    const std::vector<InstanceMeshes> instance_meshes
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
    if (auto result{pack_to_bed(to_pack, extra, {}, arrange_bed, stop_condition, settings)}) {
        return std::move(result->packs);
    }
    return std::nullopt;
}

using ArrangeLocalResult = std::optional<std::vector<std::pair<BedRef, Packs>>>;

ArrangeLocalResult arrange_local(
    StopToken stop_token,
    ProgressTracker progress_tracker,
    const Settings settings,
    const ModelInstancesPerBed model_instances_per_bed,
    const std::vector<InstanceMeshes> extra_instances
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
        if (auto packing_result{
                pack_to_bed(arrangeable, extra, fixed, bed_with_instances.arrange_bed, stop_condition, settings)
            })
        {
            packs.push_back({bed_with_instances.bed_ref, std::move(packing_result->packs)});
            extra = packing_result->unpacked_extra;
        } else {
            return std::nullopt;
        }
    }

    if (!extra.empty()) {
        // notify
    }
    return packs;
}

std::set<SelectionId> to_instance_ids(const ElementRefs& element_refs)
{
    std::set<SelectionId> result;
    for (const ElementRef& element_ref : element_refs) {
        if (element_ref.has_instance()) {
            result.insert(element_ref.instance_id);
        }
    }
    return result;
}

std::set<SelectionId> get_extra_selected_instances(
    const std::set<SelectionId>& selection,
    const ModelInstancesPerBed& model_instances_per_bed
)
{
    std::set<SelectionId> result{selection};
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
    return result;
}

} // namespace

void ArrangeInteractor::arrange(const Domain::SelectionId project_id, const Settings& settings)
{
    if (project_id == Domain::INVALID_ID) {
        return;
    }

    const double unplaced_offset{-20.0};

    JobManager& job_manager{PlatformServices::instance().job_manager()};
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
                get_arrange_bed(project_id, last_selected_bed, get_max_brim(instances), settings, m_workbench),
                get_meshes(instances, ResetTranslation::True)
            )
            .on_result([this, project_id, settings, unplaced_offset](const std::optional<Packs>& packs) {
                if (!packs) {
                    return;
                }
                apply_arrange_result(
                    project_id,
                    m_scene_interactor.bed_selection(),
                    OverflowMode::AddBeds,
                    settings.scaled_offset,
                    *packs,
                    unplaced_offset
                );
            })
            .start();
    } else if (settings.mode == Mode::Local) {
        const std::set<SelectionId> selected_model_instances{
            to_instance_ids(m_scene_interactor.object_selection().elements)
        };

        const ModelInstancesPerBed model_instances_per_bed{get_model_instances_per_bed(
            project_id,
            m_scene_interactor.bed_selection(),
            settings,
            m_workbench,
            selected_model_instances
        )};

        std::vector<InstanceMeshes> selected_extra_model_instances;
        if (!selected_model_instances.empty()) {
            const std::set<SelectionId> instance_ids{
                get_extra_selected_instances(selected_model_instances, model_instances_per_bed)
            };
            const ConstModelInstanceList instances{
                get_instances(project_id, instance_ids, m_workbench)
            };
            selected_extra_model_instances = get_meshes(instances, ResetTranslation::True);
        }

        job_manager
            .create_job(
                "arrange",
                arrange_local,
                settings,
                model_instances_per_bed,
                selected_extra_model_instances
            )
            .on_result([this, project_id, settings, unplaced_offset](
                           const ArrangeLocalResult& packs_per_bed
                       ) {
                if (!packs_per_bed) {
                    return;
                }
                double offset{unplaced_offset};

                for (const auto& [bed_ref, packs] : *packs_per_bed) {
                    BedSelection selection;
                    selection.select_one(bed_ref);
                    offset = apply_arrange_result(
                        project_id,
                        selection,
                        OverflowMode::MoveNextToFirstBed,
                        settings.scaled_offset,
                        packs,
                        offset
                    );
                }
            })
            .start();
    } else {
        PANIC("Unknown arrange mode!");
    }
}

} // namespace Slic3r::Biz
