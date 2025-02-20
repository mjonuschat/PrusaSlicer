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
using Slic3r::Biz::Slicing::Status;
using Slic3r::Tests::get_cubes_model;
using Slic3r::Tests::ModelOnBed;
using Slic3r::Tests::is_gcode_sane;
using Slic3r::Biz::Slicing::FDMResult;
using Slic3r::Biz::Slicing::FDMStatistics;
using Slic3r::Biz::Slicing::SlicingId;
using Slic3r::Domain::SelectionId;
using Slic3r::Biz::Slicing::IWipeTowerGeometryListener;
using Slic3r::Biz::Print::WipeTowerGeometry;
using Slic3r::Biz::Print::ZDepth;
using Slic3r::Tests::SlicingFixture;
using Slic3r::Tests::StatusEvent;
using Slic3r::Tests::StatusEvents;
using Slic3r::Tests::StatusListener;
using Slic3r::Biz::Slicing::ISLAResultListener;
using Slic3r::Tests::ResultListener;


TEST_CASE_METHOD(SlicingFixture, "Slice N beds", "[slicing][slicing-interactor]")
{
    ResultListener result_listener;
    slicing.add_listener(&result_listener);
    using namespace std::chrono_literals;

    std::vector<Slic3r::Tests::ModelOnBed> bed_models;

    const int beds_count{5};
    for (std::size_t i{}; i < beds_count; ++i) {
        const auto cube_count{static_cast<int>(i + 1)};
        bed_models.emplace_back(get_cubes_model(cube_count, 5));
        slicing.update_process(
            bed_models.back().model,
            bed_models.back().config,
            bed_models.back().bed_instance
        );
    }

    slicing.slice_all();

    REQUIRE(wait_for_status(dispatcher, status_listener, 3s, [](const StatusEvents &events){
        return beds_count == std::ranges::count_if(events, [](const StatusEvent& event){
            return event.status == Status::Finished;
        });
    }));

    StatusEvents update_events;
    std::ranges::copy(
        std::span{status_listener.status_events}.subspan(0, beds_count * 2),
        std::back_inserter(update_events)
    );

    StatusEvents slicing_events;
    std::ranges::copy(
        std::span{status_listener.status_events}.subspan(beds_count * 2),
        std::back_inserter(slicing_events)
    );

    StatusEvents expected_update_events;
    StatusEvents expected_slicing_events;
    for (const ModelOnBed& model_on_bed : bed_models) {
        const SelectionId bed_id{model_on_bed.bed_instance.id().id};
        expected_update_events.push_back({Status::Updating, SlicingId{0, bed_id}});
        expected_update_events.push_back({Status::Modified, SlicingId{0, bed_id}});

        expected_slicing_events.push_back({Status::Running, SlicingId{0, bed_id}});
        expected_slicing_events.push_back({Status::Finished, SlicingId{0, bed_id}});
    }

    CHECK_THAT(update_events, Equals(expected_update_events));
    CHECK_THAT(slicing_events, UnorderedEquals(expected_slicing_events));

    for (const auto& pair : result_listener.gcodes) {
        const SelectionId id{pair.first};
        const std::string& gcode{pair.second};

        const auto model_on_bed{std::ranges::find_if(bed_models, [&](const ModelOnBed& model_on_bed) {
            return model_on_bed.bed_instance.id().id == id;
        })};
        REQUIRE(model_on_bed != bed_models.end());

        const auto error{is_gcode_sane(gcode, model_on_bed->model)};
        INFO((error ? *error : ""));
        CHECK(!error);
    }
}

struct WipeTowerGeometryListener : public IWipeTowerGeometryListener
{
    void on_wipe_tower_geometry(
        WipeTowerGeometry wipe_tower_geometry, const SlicingId
    ) override
    {
        geometry = std::move(wipe_tower_geometry);
    }

    std::optional<WipeTowerGeometry> geometry;
};

TEST_CASE_METHOD(SlicingFixture, "Background process dispatches wipe_tower_geometry once available", "[slicing][slicing-callbacks]")
{
    using namespace std::chrono_literals;

    WipeTowerGeometryListener wipe_tower_geometry_listener;
    slicing.add_listener(&wipe_tower_geometry_listener);

    StatusListener status_listener;
    slicing.add_listener(&status_listener);

    auto [model, config]{Tests::load_3mf(Tests::get_datadir() / "wipe_tower.3mf")};

    ModelOnBed model_on_bed{std::move(model), std::move(config)};
    slicing.update_process(model_on_bed.model, model_on_bed.config, model_on_bed.bed_instance);
    slicing.slice_all();

    REQUIRE(wait_for_status(dispatcher, status_listener, 3s, [](const StatusEvents &events){
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
    void on_sla_result_changed(const SlicingId) override {
        this->result_recieved = true;
    };

    bool result_recieved{false};
};

TEST_CASE_METHOD(SlicingFixture, "Update reinitializes the process if printer technology differs", "[slicing][slicing-interactor]") {
    using namespace std::chrono_literals;

    using Slic3r::ConfigOptionEnum;
    using Slic3r::PrinterTechnology;

    ModelOnBed model_on_bed{get_cubes_model(1, 5)};

    SLAResultListener listener;
    slicing.add_listener(&listener);
    slicing.update_process(model_on_bed.model, model_on_bed.config, model_on_bed.bed_instance);

    model_on_bed.config.option<ConfigOptionEnum<PrinterTechnology>>("printer_technology")->value = Slic3r::ptSLA;

    slicing.update_process(model_on_bed.model, model_on_bed.config, model_on_bed.bed_instance);
    slicing.slice_all();

    REQUIRE(wait_for_status(dispatcher, status_listener, 3s, [](const StatusEvents &events){
        return events.back().status == Status::Finished;
    }));

    CHECK(listener.result_recieved);
}
