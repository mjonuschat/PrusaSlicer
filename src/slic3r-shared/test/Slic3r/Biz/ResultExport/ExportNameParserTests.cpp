#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/ResultExport/ExportNameParser.hpp"
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include "Slic3r/Domain/Types.hpp"

#include "Slic3r/App/Plater/ThumbnailImageGenerator.hpp"
#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"

#include "Slic3r/Directories.hpp"
#include "Slic3r/TestUtils/AppInstanceMessageHandlerScope.hpp"
#include "Slic3r/TestUtils/JobManagerScope.hpp"
#include "Slic3r/TestUtils/ScopedThreadDispatcher.hpp"
#include "Slic3r/TestUtils/TestData.hpp"

#include <chrono>
#include <thread>

using namespace Slic3r;
using namespace Slic3r::Biz;

namespace TriMesh = Slic3r::Biz::Algorithms::TriangleMesh;

namespace {

struct FinishedListener : public Slic3r::Biz::Slicing::IStatusListener
{
    explicit FinishedListener(Domain::SlicingId watched_id) : id(watched_id) {}

    void on_status_changed(
        const Slic3r::Biz::Slicing::StatusUpdate status_update,
        const Domain::SlicingId status_id
    ) override
    {
        if (status_id == id && status_update.code
            && *status_update.code == Slic3r::Biz::Slicing::StatusCode::Finished) {
            finished = true;
        }
    }

    Domain::SlicingId id;
    bool finished = false;
};

bool wait_until_finished(
    const FinishedListener& listener,
    const std::chrono::seconds timeout,
    Slic3r::App::Platform::StdMainThreadDispatcher& dispatcher
)
{
    const auto start = std::chrono::high_resolution_clock::now();
    while (!listener.finished) {
        dispatcher.dispatch_enqueued();
        if (std::chrono::high_resolution_clock::now() - start > timeout) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

void disable_stale_tool_overrides(Domain::ConfigPackFDM& fdm_config)
{
    for (auto& tool : fdm_config.tool) {
        for (const auto& item : tool.overrides.overridden_items()) {
            tool.overrides.disable(item.get().name());
        }
    }
}

struct ExportNameFixture
{
    ExportNameFixture()
    {
        Slic3r::set_data_dir(Tests::get_datadir().string());

        project_interactor.preset_interactor().load_preset_bundle(
            Preset::IO::BundlePaths::make_test_runtime(Tests::get_datadir())
        );
        project_interactor.new_project();
    }

    Domain::Workbench workbench;
    App::Platform::StdMainThreadDispatcher dispatcher;
    Tests::AppInstanceMessageHandlerScope app_instance_message_handler_scope{dispatcher};
    Tests::JobManagerScope job_manager_scope{dispatcher};
    App::Plater::ThumbnailImageGenerator thumbnail_image_generator;
    ProjectInteractor project_interactor{workbench, dispatcher, thumbnail_image_generator};
    Tests::ScopedThreadDispatcher thread_dispatcher{dispatcher};
};

}

TEST_CASE_METHOD(
    ExportNameFixture,
    "BOSS bed-number placeholder resolves in the exported filename",
    "[boss][export][timeout]"
)
{
    Domain::Project& project = workbench.project(project_interactor.selected_project_id());
    Domain::ConfigContainer& cc = *project.config_containers().front();
    cc.mutable_selected_preset()
        .print.config_box()
        .items.opt("output_filename_format")
        .set(std::string("{input_filename_base}_bed{bed_number}.gcode"));

    project_interactor.scene_interactor().new_object_from_mesh(
        Domain::TriangleMesh{TriMesh::make_cube(20, 20, 20)}
    );

    const Domain::SlicingId id = project_interactor.selected_bed_slicing_id();

    Domain::ConfigPack config = cc.build_print_config();
    if (auto* fdm_config = std::get_if<Domain::ConfigPackFDM>(&config)) {
        disable_stale_tool_overrides(*fdm_config);
    }
    Domain::BedInstance* bed_instance = project.find_bed_instance_by_id(id.bed_instance_id);
    REQUIRE(bed_instance != nullptr);
    project_interactor.slicing_interactor().update_process(
        project.model(),
        project.metadata(),
        cc.selected_preset().metadata(),
        config,
        *bed_instance
    );

    FinishedListener listener{id};
    project_interactor.slicing_interactor().add_listener<Slic3r::Biz::Slicing::IStatusListener>(&listener);

    project_interactor.slicing_interactor().slice_bed(id);

    REQUIRE(wait_until_finished(listener, std::chrono::seconds(60), dispatcher));

    const auto name_data = ExportNameParser::parse_export_name(project_interactor);
    REQUIRE(name_data.filename.find("_bed01.gcode") != std::string::npos);
}
