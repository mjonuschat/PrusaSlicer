#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/trompeloeil.hpp>

#include "Slic3r/TestUtils/TestData.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "libslic3r/Model.hpp"

#include <boost/filesystem/operations.hpp>


TEST_CASE("Scene Interactor Bed Tracking")
{
    using namespace Slic3r;
    using namespace Slic3r::Biz;
    using namespace trompeloeil;

    Domain::Workbench workbench;
    set_data_dir(Tests::get_datadir().string());
    workbench.load_configs();

    ProjectInteractor project_interactor{workbench};

    project_interactor.new_project();
    const auto& p = project_interactor.selected_project();
    const auto& bed = *project_interactor.selected_project().bed_container().beds().front();
    const auto& bed_center = bed.center();
    const auto& bed_size = bed.contour_aabb_extent();
    auto& scene_interactor = project_interactor.scene_interactor();

    const double cube_side = 100;
    scene_interactor.new_object_from_mesh(TriangleMesh{its_make_cube(cube_side, cube_side, cube_side)});

    // Single object amid of first bed
    Transform3d xform = Transform3d::Identity();
    xform.translate(Vec3d{bed_center.x() - cube_side / 2, bed_center.y() - cube_side / 2, 0});
    scene_interactor.transform_selection(xform.matrix());

    const auto first_el_ref = scene_interactor.selection().elements.front();

    auto& cc = p.config_containers().front();
    scene_interactor.add_bed_instance(cc->id().id);
    const auto& bed_instances = cc->bed_instances();

    // selection: instance mode
    // +y A +-----+ +-----+
    //    | | (1) | |     |
    //    | +-----+ +-----+
    //    o----->
    //         +x
    // Legend:
    //   +----+
    //   |    |     Bed
    //   +----+
    //   (1)        Selected instance
    //   [1]        Unselected instance
    REQUIRE(bed_instances.size() == 2);
    REQUIRE(p.unplaced_model_instances().empty());
    REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
    REQUIRE(bed_instances[0]->model_instances().size() == 1);
    REQUIRE(bed_instances[1]->model_instances().empty());

    Vec3d bed_pitch = bed_instances[1]->transformation().get_offset() - bed_instances[0]->transformation().get_offset();


    // Single object amid of second bed
    xform = Transform3d::Identity();
    xform.translate(Vec3d{bed_pitch.x(), 0, 0});
    scene_interactor.transform_selection(xform.matrix());

    // selection: instance mode
    // +y A +-----+ +-----+
    //    | |     | | (1) |
    //    | +-----+ +-----+
    //    o----->
    //         +x
    REQUIRE(p.unplaced_model_instances().empty());
    REQUIRE(bed_instances[0]->model_instances().empty());
    REQUIRE(bed_instances[1]->model_instances().size() == 1);
    REQUIRE(bed_instances[1]->model_instances()[0]->id().id == first_el_ref.instance_id);

    // Outside of second bed
    xform = Transform3d::Identity();
    xform.translate(Vec3d{0, bed_size.y(), 0});

    scene_interactor.transform_selection(xform.matrix());
    // selection: instance mode
    //                (1)
    // +y A +-----+ +-----+
    //    | |     | |     |
    //    | +-----+ +-----+
    //    o----->
    //         +x
    REQUIRE(p.unplaced_model_instances().size() == 1);
    REQUIRE(p.unplaced_model_instances()[0]->id().id == first_el_ref.instance_id);
    REQUIRE(bed_instances[0]->model_instances().empty());
    REQUIRE(bed_instances[1]->model_instances().empty());

    // Single object amid of second bed
    scene_interactor.add_bed_instance(cc->id().id);
    // selection: instance mode
    //                (1)
    // +y A +-----+ +-----+ +-----+
    //    | |     | |     | |     |
    //    | +-----+ +-----+ +-----+
    //    o----->
    //         +x
    REQUIRE(bed_instances.size() == 3);

    xform = Transform3d::Identity();
    xform.translate(Vec3d{0, -bed_size.y(), 0});
    scene_interactor.transform_selection(xform.matrix());

    // selection: instance mode
    // +y A +-----+ +-----+ +-----+
    //    | |     | | (1) | |     |
    //    | +-----+ +-----+ +-----+
    //    o----->
    //         +x

    // after removing middle bed
    Transform3d bed_xform = bed_instances[1]->transformation().get_matrix();
    auto old_bed_two_id = bed_instances[1]->id().id;
    scene_interactor.remove_bed_instance({cc->id().id, old_bed_two_id});

    // selection: instance mode
    // +y A +-----+ +-----+
    //    | |     | | (1) |
    //    | +-----+ +-----+
    //    o----->
    //         +x
    REQUIRE(bed_instances.size() == 2);
    REQUIRE(bed_instances[1]->id().id != old_bed_two_id);
    REQUIRE(bed_instances[1]->transformation().get_matrix().isApprox(bed_xform));

    REQUIRE(p.unplaced_model_instances().empty());
    REQUIRE(bed_instances[0]->model_instances().empty());
    REQUIRE(bed_instances[1]->model_instances().size() == 1);
    REQUIRE(bed_instances[1]->model_instances()[0]->id().id == first_el_ref.instance_id);

    // after removing bed number two again
    old_bed_two_id = bed_instances[1]->id().id;
    scene_interactor.remove_bed_instance({cc->id().id, old_bed_two_id});

    // selection: instance mode
    // +y A +-----+
    //    | |     |   (1)
    //    | +-----+
    //    o----->
    //         +x
    REQUIRE(bed_instances.size() == 1);
    REQUIRE(p.unplaced_model_instances().size() == 1);
    REQUIRE(p.unplaced_model_instances()[0]->id().id == first_el_ref.instance_id);
    REQUIRE(bed_instances[0]->model_instances().empty());

    // back to object amid first (and only) bed
    xform = Transform3d::Identity();
    xform.translate(Vec3d{-bed_pitch.x(), 0, 0});
    scene_interactor.transform_selection(xform.matrix());

    // selection: instance mode
    // +y A +-----+
    //    | | (1) |
    //    | +-----+
    //    o----->
    //         +x
    REQUIRE(bed_instances.size() == 1);
    REQUIRE(p.unplaced_model_instances().empty());
    REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
    REQUIRE(bed_instances[0]->model_instances().size() == 1);

    // back to object amid first (and only) bed
    xform = Transform3d::Identity();
    xform.translate(Vec3d{bed_center.x() - cube_side / 2 + bed_pitch.x(), bed_center.y() - cube_side / 2, 0});
    scene_interactor.add_instance(xform.matrix());
    const auto second_el_ref = scene_interactor.selection().elements.front();
    // selection: instance mode
    // +y A +-----+
    //    | | [1] |   (2)
    //    | +-----+
    //    o----->
    //         +x
    REQUIRE(p.unplaced_model_instances().size() == 1);
    REQUIRE(p.unplaced_model_instances()[0]->id().id == second_el_ref.instance_id);
    REQUIRE(bed_instances[0]->model_instances().size() == 1);
    REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);

    scene_interactor.add_bed_instance(cc->id().id);
    // selection: instance mode
    // +y a +-----+ +-----+
    //    | | [1] | | (2) |
    //    | +-----+ +-----+
    //    o----->
    //         +x
    REQUIRE(p.unplaced_model_instances().empty());
    REQUIRE(bed_instances[0]->model_instances().size() == 1);
    REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
    REQUIRE(bed_instances[1]->model_instances().size() == 1);
    REQUIRE(bed_instances[1]->model_instances()[0]->id().id == second_el_ref.instance_id);

    xform = Transform3d::Identity();
    xform.translate(Vec3d{bed_pitch.x(), 0, 0});
    scene_interactor.transform_selection(xform.matrix());

    // selection: instance mode
    // +y a   +-----+ +-----+
    //    |   | [1] | |     |  (2)
    //    |   +-----+ +-----+
    //    o----->
    //         +x
    REQUIRE(p.unplaced_model_instances().size() == 1);
    REQUIRE(p.unplaced_model_instances()[0]->id().id == second_el_ref.instance_id);
    REQUIRE(bed_instances[0]->model_instances().size() == 1);
    REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);

    xform = Transform3d::Identity();
    xform.translate(Vec3d{-bed_size.x(), 0, 0});
    scene_interactor.add_volume_from_mesh(
        TriangleMesh{its_make_cube(cube_side, cube_side, cube_side)},
        ModelVolumeType::MODEL_PART, xform
        .matrix());

    // selection: volume mode
    // +y a   +-----+ +-----+
    //    | [ |  1] | |  [  |   2]
    //    |   +-----+ +-----+
    //    o----->
    //         +x
    REQUIRE(p.unplaced_model_instances().empty());
    REQUIRE(bed_instances[0]->model_instances().size() == 1);
    REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
    REQUIRE(bed_instances[1]->model_instances().size() == 1);
    REQUIRE(bed_instances[1]->model_instances()[0]->id().id == second_el_ref.instance_id);

    xform = Transform3d::Identity();
    xform.translate(Vec3d{bed_pitch.x(), 0, 0});
    scene_interactor.transform_selection(xform.matrix());

    // selection: volume mode
    // +y a   +-----+ +-----+
    //    |   | [1] | |     |  [2]
    //    |   +-----+ +-----+
    //    o----->
    //         +x
    REQUIRE(p.unplaced_model_instances().size() == 1);
    REQUIRE(p.unplaced_model_instances()[0]->id().id == second_el_ref.instance_id);
    REQUIRE(bed_instances[0]->model_instances().size() == 1);
    REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
}

