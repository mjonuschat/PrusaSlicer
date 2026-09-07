///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Slic3r/App/Plater/ThumbnailImageGenerator.hpp"
#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/TestUtils/AppInstanceMessageHandlerScope.hpp"
#include "Slic3r/TestUtils/JobManagerScope.hpp"
#include "Slic3r/TestUtils/ScopedThreadDispatcher.hpp"
#include "Slic3r/TestUtils/TestData.hpp"
#include "Slic3r/Biz/FileLoadingLogic.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/Format/STL.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/ModelObject.hpp"
#include "Slic3r/Domain/Project.hpp"

#include "Slic3r/Directories.hpp"

#include <boost/filesystem.hpp>

using namespace Slic3r;
using namespace Slic3r::Biz;
namespace TriMesh = Slic3r::Biz::Algorithms::TriangleMesh;

namespace {

struct FileLoadingLogicFixture
{
    FileLoadingLogicFixture()
    {
        set_data_dir(Tests::get_datadir().string());
        project_interactor.preset_interactor()
            .load_preset_bundle(Preset::IO::BundlePaths::make_test_runtime(Tests::get_datadir()));
        project_interactor.new_project();
    }

    Domain::Workbench workbench;

    App::Platform::StdMainThreadDispatcher dispatcher;
    Tests::AppInstanceMessageHandlerScope app_instance_message_handler_scope{dispatcher};
    Tests::JobManagerScope job_manager_scope{dispatcher};
    App::Plater::ThumbnailImageGenerator thumbnail_image_generator;
    ProjectInteractor project_interactor{workbench, dispatcher, thumbnail_image_generator};
    Scene::SceneInteractor& scene_interactor{project_interactor.scene_interactor()};
    Tests::ScopedThreadDispatcher thread_dispatcher{dispatcher};
};

boost::filesystem::path write_temp_box_stl(double x, double y, double z)
{
    boost::filesystem::path path = boost::filesystem::temp_directory_path()
        / boost::filesystem::unique_path("boss-z-rotate-%%%%.stl");
    Domain::TriangleMesh box{TriMesh::make_cube(x, y, z)};
    REQUIRE(store_stl(path.string(), box, true));
    return path;
}

} // namespace

TEST_CASE_METHOD(
    FileLoadingLogicFixture, "Z-rotate on import rotates a bare mesh's geometry", "[boss][z-rotate]"
)
{
    boost::filesystem::path stl_path = write_temp_box_stl(20., 10., 5.);

    const Domain::ElementRefs new_instances = FileLoadingLogic::import_files_and_add_to_scene(
        {stl_path}, 1, scene_interactor, Domain::Vec2d{0., 0.}, nullptr, 90.
    );

    boost::filesystem::remove(stl_path);

    REQUIRE(new_instances.size() == 1);
    Domain::Project& project           = project_interactor.selected_project();
    const Domain::ModelObject* object  = project.find_object_by_id(new_instances.front().object_id);
    REQUIRE(object != nullptr);
    REQUIRE(object->volumes.size() == 1);

    const Domain::BoundingBox3d bbox = object->volumes.front()->mesh().bounding_box();
    const Domain::Vec3d sizes        = bbox.max - bbox.min;
    CHECK(sizes.x() == Catch::Approx(10.).margin(1e-6));
    CHECK(sizes.y() == Catch::Approx(20.).margin(1e-6));
}

TEST_CASE_METHOD(
    FileLoadingLogicFixture,
    "Z-rotate on import leaves geometry untouched when zero",
    "[boss][z-rotate]"
)
{
    boost::filesystem::path stl_path = write_temp_box_stl(20., 10., 5.);

    const Domain::ElementRefs new_instances = FileLoadingLogic::import_files_and_add_to_scene(
        {stl_path}, 1, scene_interactor, Domain::Vec2d{0., 0.}, nullptr, 0.
    );

    boost::filesystem::remove(stl_path);

    REQUIRE(new_instances.size() == 1);
    Domain::Project& project          = project_interactor.selected_project();
    const Domain::ModelObject* object = project.find_object_by_id(new_instances.front().object_id);
    REQUIRE(object != nullptr);
    REQUIRE(object->volumes.size() == 1);

    const Domain::BoundingBox3d bbox = object->volumes.front()->mesh().bounding_box();
    const Domain::Vec3d sizes        = bbox.max - bbox.min;
    CHECK(sizes.x() == Catch::Approx(20.).margin(1e-6));
    CHECK(sizes.y() == Catch::Approx(10.).margin(1e-6));
}
