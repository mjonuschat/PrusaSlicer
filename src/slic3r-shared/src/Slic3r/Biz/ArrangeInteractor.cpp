#include "Slic3r/Biz/ArrangeInteractor.hpp"
#include "Slic3r/Biz/Algorithms/ClipperUtils.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include "Slic3r/Biz/Arrange/ArrangeItem.hpp"
#include "Slic3r/Biz/Arrange/Arrange.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"

#include "libslic3r/TriangleMeshSlicer.hpp" // project_mesh

namespace Slic3r::Biz {

using Algorithms::BoundingBox::merge;
using Algorithms::BoundingBox::to_2d;
using Algorithms::ClipperUtils::diff_ex;
using Algorithms::ClipperUtils::shrink;
using Algorithms::ClipperUtils::union_ex;
using Algorithms::Point::to_2d;
using Algorithms::Scaling::scaled;
using Algorithms::Scaling::unscaled;
using Algorithms::TriangleMesh::transformed_bounding_box;
using Arrange::ArbitraryShape;
using Arrange::ArrangeItem;
using Arrange::ArrangeResult;
using Arrange::InputShape;
using Arrange::Settings;
using Arrange::to_arrange_items;
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
using Domain::ExPolygons;
using Domain::ModelInstance;
using Domain::Points;
using Domain::Polygon;
using Domain::Polygons;
using Domain::Project;
using Domain::SelectionId;
using Domain::Transform3d;
using Domain::Transformation;
using Domain::Vec2crd;
using Domain::Vec2d;
using Domain::Vec2ds;
using Domain::Vec3d;
using Domain::Workbench;

namespace {

BoundingBox3d instance_bounding_box(const Domain::ModelInstance& instance)
{
    Domain::BoundingBox3d result;
    const Transform3d inst_matrix{instance.get_transformation().get_matrix()};

    for (const Domain::ModelVolume* volume : instance.get_object()->volumes) {
        if (!volume->is_model_part()) {
            continue;
        }
        result = merge(
            result,
            transformed_bounding_box(volume->mesh(), inst_matrix * volume->get_matrix())
        );
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

ArbitraryShape extract_outline(const ModelInstance& inst)
{
    ArbitraryShape result;

    ASSERT(check_coord_bounds(to_2d(instance_bounding_box(inst))));

    for (const Domain::ModelVolume* volume : inst.get_object()->volumes) {
        const Polygons vol_outline{
            project_mesh(volume->mesh().its, inst.get_matrix() * volume->get_matrix(), [] {})
        };
        switch (volume->type()) {
        case Domain::ModelVolumeType::MODEL_PART:
            result = union_ex(result, vol_outline);
            break;
        case Domain::ModelVolumeType::NEGATIVE_VOLUME:
            result = diff_ex(result, vol_outline);
            break;
        default:;
        }
    }

    return result;
}

Points get_bed_contour(const ArrangeBed& arrange_bed)
{
    const Domain::Bed& bed{arrange_bed.bed_instance.bed};
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

std::vector<InputShape> get_arrange_input(const ConstModelInstanceList& instances)
{
    std::vector<InputShape> result;
    result.reserve(instances.size());
    for (const ModelInstance* instance : instances) {
        const ArbitraryShape outline{extract_outline(*instance)};
        result.push_back({ElementRef{instance->get_object()->id().id, instance->id().id, 0}, outline});
    }
    return result;
}

using Trafos = Scene::SceneInteractor::InstanceTransformations;

Trafos get_trafos(const std::vector<ArrangeItem>& items)
{
    Trafos result;
    for (const ArrangeItem& item : items) {
        auto trafo{Transform3d::Identity()};
        const Vec2d unscaled_translation{unscaled<double>(item.get_translation())};
        trafo.translate(Vec3d{unscaled_translation.x(), unscaled_translation.y(), 0.0});
        trafo.rotate(Eigen::AngleAxisd(item.get_rotation(), Eigen::Vector3d::UnitZ()));
        result.push_back({item.get_element_ref(), std::move(trafo.matrix())});
    }
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

} // namespace

ArrangeInteractor::ArrangeInteractor(Scene::SceneInteractor& scene_interactor, const Workbench& workbench) :
    m_scene_interactor{scene_interactor},
    m_workbench{workbench}
{}

ArrangeBeds ArrangeInteractor::get_beds(const double instances_brim, const Settings& settings) const
{
    ArrangeBeds result;
    const Project& project{m_workbench.project(m_selected_project_id)};
    for (const BedRef& bed_ref : m_scene_interactor.bed_selection().all()) {
        const ConfigContainer* config_container{
            project.find_config_container(bed_ref.config_container_id)
        };

        double brim_width{0.0};
        const ConfigPack& config{config_container->new_config()};
        if (std::holds_alternative<ConfigPackFDM>(config)) {
            const ConfigPackFDM& fdm_config{std::get<ConfigPackFDM>(config)};
            brim_width = fdm_config.print.items.opt("brim_width").get<double>();
            brim_width = std::max(brim_width, instances_brim);
        }

        const BedInstance& bed_instance{
            ASSERT_VAL(config_container)->find_bed_instance(bed_ref.instance_id)
        };
        result.push_back({bed_instance, std::max(brim_width, settings.unscaled_bed_offset)});
    };

    return result;
}

void ArrangeInteractor::arrange(const Settings& settings)
{
    if (m_selected_project_id == Domain::INVALID_ID) {
        return;
    }
    const ConstModelInstanceList instances{m_scene_interactor.selected_project_instances()};
    const ArrangeBeds arrange_beds{get_beds(get_max_brim(instances), settings)};
    const std::vector<InputShape> arrange_input{get_arrange_input(instances)};

    std::vector<ArrangeItem> to_pack{to_arrange_items(arrange_input, settings)};
    std::vector<ArrangeItem> packed;
    for (const ArrangeBed& arrange_bed : arrange_beds) {
        if (to_pack.empty()) {
            break;
        }

        ArrangeResult result{Arrange::arrange(get_bed_contour(arrange_bed), to_pack, {}, settings)};
        for (ArrangeItem& item : result.packed) {
            const Transformation bed_trafo{arrange_bed.bed_instance.transformation};
            const Vec2crd bed_offset{scaled(to_2d(bed_trafo.get_offset()))};
            item.set_translation(item.get_translation() + bed_offset);
        }
        packed.insert(packed.end(), result.packed.begin(), result.packed.end());
        to_pack = result.not_packed;
    }

    m_scene_interactor.transform_instances(get_trafos(packed));
}

void ArrangeInteractor::on_selected_project_changed(size_t index)
{
    m_selected_project_id = index;
};

} // namespace Slic3r::Biz
