#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>

#include "Slic3r/App/Plater/ThumbnailImageGenerator.hpp"
#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Biz/SecretStoreDummy.hpp"
#include "Slic3r/TestUtils/TestData.hpp"
#include "Slic3r/Domain/Model.hpp"

#include "Slic3r/Directories.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/nowide/filesystem.hpp>

namespace Slic3r::Biz::Mock {

struct SelectedProjectChangedListener : public ISelectedProjectChangedListener
{
    MAKE_MOCK1(on_selected_project_changed, void(Domain::SelectionId));
};

struct SelectedConfigContainerChangedListener : public ISelectedConfigContainerChangedListener
{
    MAKE_MOCK2(on_selected_config_container_changed, void(Domain::SelectionId, Domain::SelectionId));
};

struct SelectedBedInstancesChangedListener : public ISelectedBedInstancesChangedListener
{
    MAKE_MOCK2(
        on_selected_bed_instances_changed,
        void(Domain::SelectionId project_id, const Scene::BedSelection& selection)
    );
};
} // namespace Slic3r::Biz::Mock

void outp()
{
    std::cout << std::endl;
}

template <typename T, typename... ArgsT>
void outp(const T& arg, const ArgsT&... args)
{
    std::cout << arg;
    outp(args...);
}

TEST_CASE("Project Interactor Listeners")
{
    using namespace Slic3r::Biz;
    using namespace trompeloeil;
    namespace fs = boost::filesystem;

    boost::nowide::nowide_filesystem();

    std::unique_ptr<SecretStoreDummy> store_dummy = std::make_unique<SecretStoreDummy>();
    Platform::PlatformServices::instance().set_secret_store(std::move(store_dummy));

    Slic3r::Domain::Workbench workbench;
    Slic3r::set_data_dir(Tests::get_datadir().string());

    Slic3r::App::Platform::StdMainThreadDispatcher dispatcher;
    Slic3r::App::Plater::ThumbnailImageGenerator thumbnail_image_generator;
    ProjectInteractor project_interactor{workbench, dispatcher, thumbnail_image_generator};

    auto data_dir              = Tests::get_datadir();
    fs::path preset_bundle_dir = data_dir / "presets";
    fs::path config_dir        = data_dir / "configs";

    project_interactor.preset_interactor()
        .load_preset_bundle(preset_bundle_dir.string(), config_dir.string());

    Mock::SelectedProjectChangedListener selected_project_changed_listener;
    Mock::SelectedConfigContainerChangedListener selected_config_container_listener;
    project_interactor.add_listener<ISelectedProjectChangedListener>(
        &selected_project_changed_listener
    );
    project_interactor.add_listener<ISelectedConfigContainerChangedListener>(
        &selected_config_container_listener
    );

    Scene::SceneInteractor& scene_interactor = project_interactor.scene_interactor();
    Mock::SelectedBedInstancesChangedListener selected_bed_instances_changed_listener;
    scene_interactor.add_listener<ISelectedBedInstancesChangedListener>(
        &selected_bed_instances_changed_listener
    );

    const auto is_selection_valid{[](const Scene::BedSelection& selection) {
        const auto bed_ref{selection.last_selected_bed()};
        return bed_ref.config_container_id > 0 && bed_ref.instance_id > 0;
    }};

    {
        REQUIRE_CALL(selected_project_changed_listener, on_selected_project_changed(0));
        REQUIRE_CALL(selected_config_container_listener, on_selected_config_container_changed(0, gt(0)));
        REQUIRE_CALL(
            selected_bed_instances_changed_listener,
            on_selected_bed_instances_changed(0, _)
        ).WITH(is_selection_valid(_2));
        project_interactor.new_project();
    }

    {
        REQUIRE_CALL(selected_project_changed_listener, on_selected_project_changed(1));
        REQUIRE_CALL(selected_config_container_listener, on_selected_config_container_changed(1, gt(0)))
            .SIDE_EFFECT(
                auto capStr = std::string("selected_config_container( cc: ") + std::to_string(_2) + " )";
                UNSCOPED_INFO(capStr);
            );
        REQUIRE_CALL(selected_bed_instances_changed_listener, on_selected_bed_instances_changed(1, _))
            .WITH(is_selection_valid(_2))
            .SIDE_EFFECT(
                auto capStr = std::string("selected_bed_instance( cc: ")
                    + std::to_string(_2.last_selected_bed().config_container_id)
                    + " )";
                UNSCOPED_INFO(capStr);
            );
        project_interactor.new_project();
    }

    {
        REQUIRE_CALL(selected_project_changed_listener, on_selected_project_changed(0))
            .SIDE_EFFECT(
                auto capStr = std::string("selected_project: ") + std::to_string(_1);
                UNSCOPED_INFO(capStr);
            );
        REQUIRE_CALL(selected_config_container_listener, on_selected_config_container_changed(0, gt(0)))
            .SIDE_EFFECT(
                auto capStr = std::string("selected_config_container( cc: ") + std::to_string(_2) + " )";
                UNSCOPED_INFO(capStr);
            );
        // on_selected_bed_instances_changed is not called as it is selected during new_project()
        project_interactor.select_project(0);
    }

    // Queue must be clear before ProjectInteractor can be destroyed.
    dispatcher.close();
}
