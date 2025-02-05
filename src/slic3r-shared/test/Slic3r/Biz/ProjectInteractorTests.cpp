#include "Slic3r/Biz/IProjectsChangedListener.hpp"

#include <catch2/catch.hpp>

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "libslic3r/Model.hpp"

//#include "Slic3r/Biz/ListenerSpy.hpp"


namespace mock {
struct MockSelectedProjectChangedListener : public Slic3r::Biz::ISelectedProjectChangedListener
{
#if 1
    using on_selected_project_changed_args_vec_t = std::vector<std::tuple<size_t>>;
    on_selected_project_changed_args_vec_t on_selected_project_changed_args;

    void on_selected_project_changed(size_t arg0) override
    {on_selected_project_changed_args.emplace_back(arg0);}
#else
    DEFINE_SPY_METHOD(on_selected_project_changed, void(size_t))
#endif
};
}

TEST_CASE("Project Interactor Listeners", "[ProjectInteractor]")
{
    Slic3r::Domain::Workbench workbench;
    boost::filesystem::path data_path = boost::filesystem::path(__FILE__).parent_path() / "../../data";
    Slic3r::set_data_dir(data_path.string());
    workbench.load_configs();

    Slic3r::Biz::ProjectInteractor project_interactor{workbench};
    mock::MockSelectedProjectChangedListener selected_project_changed_listener;
    project_interactor.add_selected_project_changed_listener(&selected_project_changed_listener);

    project_interactor.new_project();
    REQUIRE(selected_project_changed_listener.on_selected_project_changed_args.size() == 1);
    project_interactor.new_project();
    REQUIRE(selected_project_changed_listener.on_selected_project_changed_args.size() == 2);

}
