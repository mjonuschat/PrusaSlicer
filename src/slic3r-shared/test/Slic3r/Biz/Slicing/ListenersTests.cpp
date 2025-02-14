#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <span>
#include <fstream>

#include <Slic3r/Biz/Slicing/SlicingInteractor.hpp>

#include "Slic3r/Biz/Slicing/TestUtils.hpp"
#include "Slic3r/Biz/Slicing/GCodeUtils.hpp"
#include "Slic3r/TestUtils/TestData.hpp"

using namespace Catch;
using Catch::Matchers::Equals;
using Catch::Matchers::UnorderedEquals;

using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::high_resolution_clock;
using Slic3r::Biz::Slicing::SlicingInteractor;
using Slic3r::Biz::Slicing::IFDMResultListener;
using Slic3r::Biz::Slicing::Status;
using Slic3r::Tests::get_config;
using Slic3r::Tests::generate_cubes;
using Slic3r::Tests::is_gcode_sane;
using Slic3r::App::Platform::StdMainThreadDispatcher;
using Slic3r::Biz::Platform::PlatformServices;
using Slic3r::Biz::Slicing::FDMResult;
using Slic3r::Biz::Slicing::FDMStatistics;
using Slic3r::Biz::Slicing::ProjectBedId;
using Slic3r::Domain::SelectionId;
using Slic3r::Biz::Slicing::IWipeTowerGeometryListener;
using Slic3r::Biz::Print::WipeTowerGeometry;
using Slic3r::Biz::Print::ZDepth;
using GCodes = std::map<ProjectBedId, std::string>;
using Slic3r::Tests::SlicingFixture;
using Slic3r::Tests::StatusEvent;
using Slic3r::Tests::StatusEvents;
using Slic3r::Tests::StatusListener;
using Slic3r::Biz::Slicing::ISLAResultListener;

struct ResultListener : public IFDMResultListener
{
    void on_fdm_result_changed(
        std::shared_ptr<FDMResult> result, std::shared_ptr<FDMStatistics>, const ProjectBedId id
    ) override
    {
        std::ifstream file{result->filename};
        std::stringstream buffer;
        buffer << file.rdbuf();
        gcodes[id] = buffer.str();

        boost::system::error_code error_code;
        boost::filesystem::remove(result->filename, error_code);
        result->reset();
    }

    GCodes gcodes;
};

TEST_CASE_METHOD(SlicingFixture, "Slice N beds", "[slicing][slicing-interactor]")
{

    ResultListener result_listener;
    slicing.add_listener(&result_listener);
    using namespace std::chrono_literals;

    const int bed_count{5};
    for (int cube_count{1}; cube_count <= bed_count; ++cube_count) {
        const auto bed_id{static_cast<SelectionId>(cube_count)};
        const ProjectBedId id{0, bed_id};
        models[id] = generate_cubes(cube_count, 5);
        slicing.update_bed(models[id], get_config(), bed_id);
    }

    slicing.slice_all();

    REQUIRE(wait_for_status(dispatcher, status_listener, 10s, [](const StatusEvents &events){
        return bed_count == std::ranges::count_if(events, [](const StatusEvent& event){
            return event.status == Status::Finished;
        });
    }));

    StatusEvents update_events;
    std::ranges::copy(
        std::span{status_listener.status_events}.subspan(0, bed_count * 2),
        std::back_inserter(update_events)
    );

    StatusEvents slicing_events;
    std::ranges::copy(
        std::span{status_listener.status_events}.subspan(bed_count * 2),
        std::back_inserter(slicing_events)
    );

    StatusEvents expected_update_events;
    StatusEvents expected_slicing_events;
    for (std::size_t bed_id{1}; bed_id <= bed_count; ++bed_id) {
        expected_update_events.push_back({Status::Updating, ProjectBedId{0, bed_id}});
        expected_update_events.push_back({Status::Modified, ProjectBedId{0, bed_id}});

        expected_slicing_events.push_back({Status::Running, ProjectBedId{0, bed_id}});
        expected_slicing_events.push_back({Status::Finished, ProjectBedId{0, bed_id}});
    }

    CHECK_THAT(update_events, Equals(expected_update_events));
    CHECK_THAT(slicing_events, UnorderedEquals(expected_slicing_events));

    for (const auto &[id, gcode] : result_listener.gcodes) {
        const auto error{is_gcode_sane(gcode, models[id])};
        INFO((error ? *error : ""));
        CHECK(!error);
    }
}

struct WipeTowerGeometryListener : public IWipeTowerGeometryListener
{
    void on_wipe_tower_geometry(
        WipeTowerGeometry wipe_tower_geometry, const ProjectBedId
    ) override
    {
        geometry = std::move(wipe_tower_geometry);
    }

    std::optional<WipeTowerGeometry> geometry;
};

TEST_CASE("Background process dispatches wipe_tower_geometry once available", "[slicing][slicing-callbacks]")
{
    using namespace std::chrono_literals;

    StdMainThreadDispatcher dispatcher{};
    PlatformServices::instance().set_main_thread_dispatcher(&dispatcher);

    SlicingInteractor slicing;
    slicing.on_selected_project_changed(0);

    WipeTowerGeometryListener wipe_tower_geometry_listener;
    slicing.add_listener(&wipe_tower_geometry_listener);

    StatusListener status_listener;
    slicing.add_listener(&status_listener);

    auto [model, config]{Tests::load_3mf(Tests::get_datadir() / "wipe_tower.3mf")};
    slicing.update_bed(model, config, 0);
    slicing.slice_all();

    REQUIRE(wait_for_status(dispatcher, status_listener, 10s, [](const StatusEvents &events){
        return events.back().status == Status::Finished;
    }));

    REQUIRE(wipe_tower_geometry_listener.geometry);
    const WipeTowerGeometry geometry{*wipe_tower_geometry_listener.geometry};

    // Two step wipe tower with brim.
    REQUIRE(geometry.size() == 4);
    CHECK(geometry[0].z == 0);
    CHECK(geometry[3].depth == 0);

    ZDepth previous_z_depth{geometry.front()};
    for (const auto& [z, depth] : std::span{geometry}.subspan(1)) {
        CHECK(previous_z_depth.z < z);
        CHECK(previous_z_depth.depth > depth);
        previous_z_depth = {z, depth};
    }
}

struct SLAResultListener : public ISLAResultListener {
    void on_sla_result_changed(const ProjectBedId) override {
        this->result_recieved = true;
    };

    bool result_recieved{false};
};

TEST_CASE_METHOD(SlicingFixture, "Update reinitializes the process if printer technology differs", "[slicing][slicing-interactor]") {
    using namespace std::chrono_literals;

    const ProjectBedId id{0, 0};
    Slic3r::DynamicPrintConfig config{get_config()};

    using Slic3r::ConfigOptionEnum;
    using Slic3r::PrinterTechnology;

    SLAResultListener listener;
    slicing.add_listener(&listener);

    models[id] = generate_cubes(10, 5);
    slicing.update_bed(models[id], config, id.bed_id);

    config.option<ConfigOptionEnum<PrinterTechnology>>("printer_technology")->value = Slic3r::ptSLA;

    slicing.update_bed(models[id], config, id.bed_id);
    slicing.slice_all();

    REQUIRE(wait_for_status(dispatcher, status_listener, 10s, [](const StatusEvents &events){
        return events.back().status == Status::Finished;
    }));

    CHECK(listener.result_recieved);
}
