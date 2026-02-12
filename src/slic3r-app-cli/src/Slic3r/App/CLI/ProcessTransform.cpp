#include "Slic3r/App/CLI/ProcessTransform.hpp"

#include "CLIUtils.hpp"

#include "Slic3r/App/Init.hpp"
#include "Slic3r/Biz/Arrange/Arrange.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/Model.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/Utils/CutUtils.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/ModelInstance.hpp"
#include "Slic3r/Domain/ModelObject.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/Transformation.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Math.hpp"

#include <spdlog/spdlog.h>

#include "libslic3r/ModelProcessing.hpp"

using Slic3r::App::InitParams;
using Slic3r::Domain::Model;
using Slic3r::Domain::ModelInstance;
using Slic3r::Domain::ModelObject;
using Slic3r::Domain::ModelObjectPtrs;
using Slic3r::Domain::Project;
using Slic3r::Domain::translation_transform;
using Slic3r::Domain::Vec2crd;
using Slic3r::Domain::Vec3d;
using Slic3r::Domain::Vec2d;

using namespace Slic3r;
using namespace Slic3r::Biz;

namespace Slic3r::App::CLI {

bool process_transform(
    const InitParams& init_params,
    const Domain::ConfigPack& config_pack,
    std::vector<Project>& projects
)

{
    const App::TransformParams& transform = init_params.transform;

    if (transform.merge.has_value() && transform.merge.value() && !projects.empty()) {
        Project& merged_project = projects.front();
        Model& merged_model     = merged_project.model();
        for (Project& project : std::ranges::subrange(projects.begin() + 1, projects.end())) {
            for (ModelObject* o : project.model().objects) {
                merged_model.add_object(*o);
            }
        }

        // Rearrange instances unless --dont-arrange is supplied
        if (!transform.dont_arrange.has_value() && !transform.dont_arrange.value()) {
            Biz::Arrange::arrange_model_in_place(
                merged_model,
                get_bed_shape(config_pack),
                Biz::Arrange::Settings{
                    .scaled_offset = double(scale_(min_object_distance(config_pack) / 2.))
                }
            );
        }

        projects.resize(1);
    }

    if (transform.duplicate.has_value()) {
        for (Project& project : projects) {
            Model &model = project.model();
            const bool all_objects_have_instances = std::none_of(
                model.objects.begin(),
                model.objects.end(),
                [](ModelObject* o) { return o->instances.empty(); }
            );

            const uint32_t dups = transform.duplicate.value();
            if (!all_objects_have_instances) {
                model.add_default_instances();
            }

            if (dups > 1) {
                for (ModelObject* object : model.objects) {
                    for (uint32_t i = 1; i < dups; ++i) {
                        object->add_instance(*object->instances.front());
                    }
                }
            }

            Biz::Arrange::arrange_model_in_place(
                model,
                get_bed_shape(config_pack),
                Biz::Arrange::Settings{
                    .scaled_offset = double(scale_(min_object_distance(config_pack) / 2.))
                }
            );
        }
    }

    if (transform.duplicate_grid.has_value()) {
        const std::array<uint32_t, 2>& ints = transform.duplicate_grid.value();
        const int x                         = ints.size() > 0 ? ints.at(0) : 1;
        const int y                         = ints.size() > 1 ? ints.at(1) : 1;
        const double distance = 6.; // TODO: duplicate_distance was removed in the new configs.
        for (Project& project : projects) {
            Algorithms::Model::duplicate_objects_grid(
                project.model(),
                x,
                y,
                (distance > 0) ? distance : 6
            ); // TODO: this is not the right place for setting a default
        }
    }

    if (transform.center.has_value()) {
        for (Project& project : projects) {
            Model &model = project.model();
            model.add_default_instances();
            // this affects instances:
            Algorithms::Model::center_instances_around_point(model, transform.center.value());
            // this affects volumes:
            // FIXME Vojtech: Who knows why the complete model should be aligned with Z as a single rigid body?
            // model.align_to_ground();
            BoundingBoxf3 bbox;
            for (ModelObject* model_object : model.objects) {
                // We are interested into the Z span only, therefore it is sufficient to measure the bounding box of the 1st instance only.
                bbox = Algorithms::BoundingBox::merge(
                    bbox,
                    Algorithms::ModelObject::instance_bounding_box(*model_object, 0, false)
                );
            }

            for (ModelObject* model_object : model.objects) {
                for (ModelInstance* model_instance : model_object->instances) {
                    model_instance->set_offset(
                        Domain::Z,
                        model_instance->get_offset(Domain::Z) - bbox.min.z()
                    );
                }
            }
        }
    }

    if (transform.align_xy.has_value()) {
        const Vec2d& p = transform.align_xy.value();
        for (Project& project : projects) {
            Model &model = project.model();
            BoundingBoxf3 bb = Algorithms::Model::bounding_box_exact(model);
            // this affects volumes:
            Algorithms::Model::translate(
                model,
                -(bb.min.x() - p.x()),
                -(bb.min.y() - p.y()),
                -bb.min.z()
            );
        }
    }

    if (transform.rotation.has_value()) {
        const Vec3d& rotation = transform.rotation.value();
        for (Project& project : projects) {
            for (ModelObject* o : project.model().objects) {
                // This all affects volumes.
                if (rotation.z() != 0.) {
                    Algorithms::ModelObject::rotate(*o, deg2rad(rotation[2]), Domain::Z);
                }

                if (rotation.x() != 0.) {
                    Algorithms::ModelObject::rotate(*o, deg2rad(rotation[0]), Domain::X);
                }

                if (rotation.y() != 0.) {
                    Algorithms::ModelObject::rotate(*o, deg2rad(rotation[1]), Domain::Y);
                }
            }
        }
    }

    if (transform.scale.has_value()) {
        for (Project& project : projects) {
            for (ModelObject* o : project.model().objects) {
                // this affects volumes:
                Algorithms::ModelObject::scale(*o, transform.scale->get_abs_value(1.));
            }
        }
    }

    if (transform.scale_to_fit.has_value()) {
        for (Project& project : projects) {
            for (ModelObject* o : project.model().objects) {
                // this affects volumes:
                Algorithms::ModelObject::scale_to_fit(*o, transform.scale_to_fit.value());
            }
        }
    }

    if (transform.cut_z.has_value()) {
        const Vec3d plane_center = transform.cut_z.value() * Vec3d::UnitZ();
        for (Project& project : projects) {
            Model &model = project.model();
            Model new_model;
            Algorithms::Model::translate(
                model,
                0,
                0,
                -Algorithms::Model::bounding_box_exact(model).min.z()
            ); // align to z = 0
            size_t num_objects = model.objects.size();
            for (size_t i = 0; i < num_objects; ++i) {
                ModelObject* mo               = model.objects.front();
                const Vec3d cut_center_offset = plane_center - mo->instances[0]->get_offset();
                Cut cut(
                    mo,
                    0,
                    translation_transform(cut_center_offset),
                    {.keep_upper = true, .keep_lower = true, .place_on_cut_upper = true}
                );
                auto cut_objects = cut.perform_with_plane();
                for (ModelObject* obj : cut_objects) {
                    new_model.add_object(*obj);
                }

                model.delete_object(size_t(0));
            }

            project.model() = new_model;
        }
    }

    if (transform.split.has_value() && transform.split.value()) {
        for (Project& project : projects) {
            Model &model = project.model();
            size_t num_objects = model.objects.size();
            for (size_t i = 0; i < num_objects; ++i) {
                ModelObjectPtrs new_objects;
                ModelProcessing::split(model.objects.front(), &new_objects);
                model.delete_object(size_t(0));
            }
        }
    }

    // All transforms have been dealt with. Now ensure that the objects are on bed.
    // (Unless the user said otherwise.)
    if (!transform.ensure_on_bed.has_value() || transform.ensure_on_bed.value()) {
        for (Project& project : projects) {
            for (ModelObject* o : project.model().objects) {
                Algorithms::ModelObject::ensure_on_bed(*o);
            }
        }
    }

    return true;
}

} // namespace Slic3r::App::CLI
