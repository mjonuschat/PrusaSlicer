#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/TestUtils/TestData.hpp"
#include "libslic3r/Model.hpp"

#include <boost/filesystem/operations.hpp>

namespace Slic3r::Biz::Mock {

struct SelectedProjectChangedListener : public ISelectedProjectChangedListener
{
    MAKE_MOCK1(on_selected_project_changed, void(Domain::SelectionId));
};

struct SelectedConfigContainerChangedListener : public ISelectedConfigContainerChangedListener
{
    MAKE_MOCK2(on_selected_config_container_changed, void(Domain::SelectionId, Domain::SelectionId));
};


struct SelectedBedInstanceChangedListener : public ISelectedBedInstanceChangedListener
{
    MAKE_MOCK3(on_selected_bed_instance_changed, void(Domain::SelectionId project_id, Domain::SelectionId container_id, Domain::SelectionId bed_instance_id));
};

struct Dispatcher : public Platform::IMainThreadDispatcher {
    void dispatch_on_main_thread(Function func) override {}
    void dispatch_on_main_thread_after(Function func) override {}
    bool dispatch_enqueued() override {return false;}
};
}

void outp() { std::cout << std::endl; }

template <typename T, typename ... ArgsT>
void outp(const T& arg, const ArgsT& ... args)
{
    std::cout << arg;
    outp(args...);
}



TEST_CASE("Project Interactor Listeners")
{
    using namespace Slic3r::Biz;
    using namespace trompeloeil;

    auto dispatcher{std::make_unique<Mock::Dispatcher>()};
    Platform::PlatformServices::instance().set_main_thread_dispatcher(dispatcher.get());

    Slic3r::Domain::Workbench workbench;
    Slic3r::set_data_dir(Tests::get_datadir().string());
    workbench.load_configs();

    ProjectInteractor project_interactor{workbench};

    Mock::SelectedProjectChangedListener selected_project_changed_listener;
    Mock::SelectedConfigContainerChangedListener selected_config_container_listener;
    project_interactor.add_selected_project_changed_listener(&selected_project_changed_listener);
    project_interactor.add_selected_config_container_changed_listener(&selected_config_container_listener);

    Scene::SceneInteractor& scene_interactor = project_interactor.scene_interactor();
    Mock::SelectedBedInstanceChangedListener selected_bed_instance_changed_listener;
    scene_interactor.add_bed_instance_selection_changed_listener(&selected_bed_instance_changed_listener);

    {
        REQUIRE_CALL(selected_project_changed_listener, on_selected_project_changed(0));
        REQUIRE_CALL(selected_config_container_listener, on_selected_config_container_changed(0, gt(0)));
        REQUIRE_CALL(selected_bed_instance_changed_listener, on_selected_bed_instance_changed(0, gt(0), gt(0)));
        project_interactor.new_project();
    }

    {
        REQUIRE_CALL(selected_project_changed_listener, on_selected_project_changed(1));
        REQUIRE_CALL(
            selected_config_container_listener,
            on_selected_config_container_changed(1, gt(0))
        )
        .SIDE_EFFECT(
            auto capStr = std::string("selected_config_container( cc: ") + std::to_string(_2) + " )";
            UNSCOPED_INFO(capStr);
        );
        REQUIRE_CALL(
            selected_bed_instance_changed_listener,
            on_selected_bed_instance_changed(1, gt(0), gt(0))
        )
        .SIDE_EFFECT(
            auto capStr = std::string("selected_bed_instance( cc: ") + std::to_string(_2) + " )";
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
        // REQUIRE_CALL(selected_config_container_listener, on_selected_config_container_changed(1, gt(0)))
        // .SIDE_EFFECT(
        //     auto capStr = std::string("selected_config_container( cc: ") + std::to_string(_2) + " )";
        //     UNSCOPED_INFO(capStr);
        // );
        project_interactor.select_project(0);
    }

}
