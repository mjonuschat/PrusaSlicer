#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/trompeloeil.hpp>

#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/TestUtils/TestData.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "libslic3r/Model.hpp"

struct SlicingInputChangedListener : Slic3r::Biz::ISlicingInputChangedListener
{
    MAKE_MOCK1(on_slicing_input_changed, void(const Slic3r::Domain::BedRef&));
    MAKE_MOCK1(on_slicing_input_removed, void(const Slic3r::Domain::BedRef&));
};

struct ScopedThreadDispatcher
{
    ScopedThreadDispatcher(Slic3r::Biz::Platform::IMainThreadDispatcher& dispatcher)
        : m_dispatcher(dispatcher)
    {}
    ~ScopedThreadDispatcher() { m_dispatcher.close(); }

private:
    Slic3r::Biz::Platform::IMainThreadDispatcher& m_dispatcher;
};


TEST_CASE("Scene Interactor Bed Tracking")
{
    using namespace Slic3r;
    using namespace Slic3r::Biz;
    using namespace trompeloeil;

    SlicingInputChangedListener slicing_input_changed_listener;
    Domain::Workbench workbench;
    set_data_dir(Tests::get_datadir().string());
    workbench.load_configs();

    App::Platform::StdMainThreadDispatcher dispatcher;
    ProjectInteractor project_interactor{workbench, dispatcher};
    ScopedThreadDispatcher thread_dispatcher{dispatcher};

    project_interactor.scene_interactor().add_listener<ISlicingInputChangedListener>(&slicing_input_changed_listener);
    {
        ALLOW_CALL(slicing_input_changed_listener, on_slicing_input_changed(trompeloeil::_));
        project_interactor.new_project();
        // REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(gt(0)))
        // .SIDE_EFFECT({ std::cout << _1 << std::endl; });
    }

    const auto& p = project_interactor.selected_project();
    const auto& bed = *project_interactor.selected_project().bed_container().beds().front();
    const auto& bed_center = bed.center();
    const auto& bed_size = bed.contour_aabb_extent();
    auto& scene_interactor = project_interactor.scene_interactor();

    auto& cc = p.config_containers().front();
    const auto& bed_instances = cc->bed_instances();
    const auto bi1_id = bed_instances[0]->id().id;
    const double cube_side = 100; // mm
    {
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi1_id);
        scene_interactor.new_object_from_mesh(TriangleMesh{its_make_cube(cube_side, cube_side, cube_side)});
    }

    Transform3d xform = Transform3d::Identity();
    // Single object amid of first bed
    {
        xform.translate(Vec3d{bed_center.x() - cube_side / 2, bed_center.y() - cube_side / 2, 0});
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi1_id);
        scene_interactor.transform_selection(xform.matrix());
    }

    const auto first_el_ref = scene_interactor.selection().elements.front();

    {
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(ANY(const Domain::BedRef&)));
        scene_interactor.add_bed_instance(cc->id().id);

        // selection: instance mode
        // +y A +-<1>-+ +-<2>-+
        //    | | (1) | |     |
        //    | +-----+ +-----+
        //    o----->
        //         +x
        // Legend:
        //   +-<1>-+
        //   |     |     Bed (with ID symbol <1> --- i.e. id is stored in bi1_id)
        //   +-----+
        //   (1)        Selected instance
        //   [1]        Unselected instance
        REQUIRE(bed_instances.size() == 2);
        REQUIRE(p.unplaced_model_instances().empty());
        REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
        REQUIRE(bed_instances[0]->model_instances().size() == 1);
        REQUIRE(bed_instances[1]->model_instances().empty());
    };

    Vec3d bed_pitch = bed_instances[1]->transformation().get_offset() - bed_instances[0]->transformation().get_offset();
    const auto bi2_id = bed_instances[1]->id().id;

    {
        // Single object amid of second bed
        xform = Transform3d::Identity();
        xform.translate(Vec3d{bed_pitch.x(), 0, 0});

        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi1_id);
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi2_id);

        scene_interactor.transform_selection(xform.matrix());

        // selection: instance mode
        // +y A +-<1>-+ +-<2>-+
        //    | |     | | (1) |
        //    | +-----+ +-----+
        //    o----->
        //         +x
        REQUIRE(p.unplaced_model_instances().empty());
        REQUIRE(bed_instances[0]->model_instances().empty());
        REQUIRE(bed_instances[1]->model_instances().size() == 1);
        REQUIRE(bed_instances[1]->model_instances()[0]->id().id == first_el_ref.instance_id);
    }
    {
        // Outside of second bed
        xform = Transform3d::Identity();
        xform.translate(Vec3d{0, bed_size.y(), 0});

        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi2_id);

        scene_interactor.transform_selection(xform.matrix());
        // selection: instance mode
        //                (1)
        // +y A +-<1>-+ +-<2>-+
        //    | |     | |     |
        //    | +-----+ +-----+
        //    o----->
        //         +x
        REQUIRE(p.unplaced_model_instances().size() == 1);
        REQUIRE(p.unplaced_model_instances()[0]->id().id == first_el_ref.instance_id);
        REQUIRE(bed_instances[0]->model_instances().empty());
        REQUIRE(bed_instances[1]->model_instances().empty());
    }

    {
        // Single object amid of second bed
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(ANY(const Domain::BedRef&)));
        scene_interactor.add_bed_instance(cc->id().id);
        // selection: instance mode
        //                (1)
        // +y A +-<1>-+ +-<2>-+ +-<3>-+
        //    | |     | |     | |     |
        //    | +-----+ +-----+ +-----+
        //    o----->
        //         +x
        REQUIRE(bed_instances.size() == 3);
    }

    const auto bi3_id = bed_instances[2]->id().id;
    {
        xform = Transform3d::Identity();
        xform.translate(Vec3d{0, -bed_size.y(), 0});
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi2_id);
        scene_interactor.transform_selection(xform.matrix());


        // selection: instance mode
        // +y A +-<1>-+ +-<2>-+ +-<3>-+
        //    | |     | | (1) | |     |
        //    | +-----+ +-----+ +-----+
        //    o----->
        //         +x
    }

    auto old_bed_two_id = bed_instances[1]->id().id;
    {
        // after removing middle bed
        Transform3d bed_xform = bed_instances[1]->transformation().get_matrix();
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi3_id);
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_removed(_))
            .WITH(_1.instance_id == bi2_id);
        scene_interactor.remove_bed_instance({cc->id().id, old_bed_two_id});

        // selection: instance mode
        // +y A +-<1>-+ +-<3>-+
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
    }
    {
        // after removing bed number two again
        old_bed_two_id = bed_instances[1]->id().id;
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_removed(_))
            .WITH(_1.instance_id == bi3_id);
        scene_interactor.remove_bed_instance({cc->id().id, old_bed_two_id});

        // selection: instance mode
        // +y A +-<1>-+
        //    | |     |   (1)
        //    | +-----+
        //    o----->
        //         +x
        REQUIRE(bed_instances.size() == 1);
        REQUIRE(p.unplaced_model_instances().size() == 1);
        REQUIRE(p.unplaced_model_instances()[0]->id().id == first_el_ref.instance_id);
        REQUIRE(bed_instances[0]->model_instances().empty());
    }
    {
        // back to object amid first (and only) bed
        xform = Transform3d::Identity();
        xform.translate(Vec3d{-bed_pitch.x(), 0, 0});
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi1_id);
        scene_interactor.transform_selection(xform.matrix());

        // selection: instance mode
        // +y A +-<1>-+
        //    | | (1) |
        //    | +-----+
        //    o----->
        //         +x
        REQUIRE(bed_instances.size() == 1);
        REQUIRE(p.unplaced_model_instances().empty());
        REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
        REQUIRE(bed_instances[0]->model_instances().size() == 1);
    }
    Domain::ElementRef second_el_ref;
    {
        // back to object amid first (and only) bed
        xform = Transform3d::Identity();
        xform.translate(Vec3d{bed_center.x() - cube_side / 2 + bed_pitch.x(), bed_center.y() - cube_side / 2, 0});
        scene_interactor.add_instance(xform.matrix());
        second_el_ref = scene_interactor.selection().elements.front();
        // selection: instance mode
        // +y A +-<1>-+
        //    | | [1] |   (2)
        //    | +-----+
        //    o----->
        //         +x
        REQUIRE(p.unplaced_model_instances().size() == 1);
        REQUIRE(p.unplaced_model_instances()[0]->id().id == second_el_ref.instance_id);
        REQUIRE(bed_instances[0]->model_instances().size() == 1);
        REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
    }
    {
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_));
        scene_interactor.add_bed_instance(cc->id().id);
        // selection: instance mode
        // +y a +-<1>-+ +-<4>-+
        //    | | [1] | | (2) |
        //    | +-----+ +-----+
        //    o----->
        //         +x
        REQUIRE(p.unplaced_model_instances().empty());
        REQUIRE(bed_instances[0]->model_instances().size() == 1);
        REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
        REQUIRE(bed_instances[1]->model_instances().size() == 1);
        REQUIRE(bed_instances[1]->model_instances()[0]->id().id == second_el_ref.instance_id);
    }
    const auto bi4_id = bed_instances[1]->id().id;
    {
        xform = Transform3d::Identity();
        xform.translate(Vec3d{bed_pitch.x(), 0, 0});
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi4_id);
        scene_interactor.transform_selection(xform.matrix());

        // selection: instance mode
        // +y a   +-<1>-+ +-<4>-+
        //    |   | [1] | |     |  (2)
        //    |   +-----+ +-----+
        //    o----->
        //         +x
        REQUIRE(p.unplaced_model_instances().size() == 1);
        REQUIRE(p.unplaced_model_instances()[0]->id().id == second_el_ref.instance_id);
        REQUIRE(bed_instances[0]->model_instances().size() == 1);
        REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
    }
    {
        xform = Transform3d::Identity();
        xform.translate(Vec3d{-bed_size.x(), 0, 0});

        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi1_id);
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi4_id);

        scene_interactor.add_volume_from_mesh(
            TriangleMesh{its_make_cube(cube_side, cube_side, cube_side)},
            ModelVolumeType::MODEL_PART,
            xform.matrix()
        );

        // selection: volume mode
        // +y a   +-<1>-+ +-<4>-+
        //    | [ |  1] | |  [  |   2]
        //    |   +-----+ +-----+
        //    o----->
        //         +x
        REQUIRE(p.unplaced_model_instances().empty());
        REQUIRE(bed_instances[0]->model_instances().size() == 1);
        REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
        REQUIRE(bed_instances[1]->model_instances().size() == 1);
        REQUIRE(bed_instances[1]->model_instances()[0]->id().id == second_el_ref.instance_id);
    }
    {
        xform = Transform3d::Identity();
        xform.translate(Vec3d{bed_pitch.x(), 0, 0});
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi1_id);
        REQUIRE_CALL(slicing_input_changed_listener, on_slicing_input_changed(_))
            .WITH(_1.instance_id == bi4_id);
        scene_interactor.transform_selection(xform.matrix());

        // selection: volume mode
        // +y a   +-<1>-+ +-<4>-+
        //    |   | [1] | |     |  [2]
        //    |   +-----+ +-----+
        //    o----->
        //         +x
        REQUIRE(p.unplaced_model_instances().size() == 1);
        REQUIRE(p.unplaced_model_instances()[0]->id().id == second_el_ref.instance_id);
        REQUIRE(bed_instances[0]->model_instances().size() == 1);
        REQUIRE(bed_instances[0]->model_instances()[0]->id().id == first_el_ref.instance_id);
    }
    // Queue must be clear before ProjectInteractor can be destroyed.
    //dispatcher.close();
}

