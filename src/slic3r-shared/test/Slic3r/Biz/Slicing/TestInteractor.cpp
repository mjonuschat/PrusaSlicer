#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <ranges>
#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <span>
#include <fstream>

#include <Slic3r/Biz/Slicing/SlicingInteractor.hpp>
#include <Slic3r/Biz/Platform/PlatformServices.hpp>
#include <Slic3r/App/Platform/StdMainThreadDispatcher.hpp>

#include "Slic3r/Biz/Slicing/TestUtils.hpp"
#include "Slic3r/Biz/Slicing/GCodeUtils.hpp"
#include "Slic3r/Log.hpp"

using namespace Catch;
using Catch::Matchers::Equals;
using Catch::Matchers::UnorderedEquals;
using Catch::Matchers::Contains;

using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::high_resolution_clock;
using Slic3r::Biz::Slicing::SlicingInteractor;
using Slic3r::Biz::Slicing::ISlicingStatusListener;
using Slic3r::Biz::Slicing::ISlicingResultListener;
using Slic3r::Biz::Slicing::Status;
using Slic3r::Tests::get_config;
using Slic3r::Tests::generate_cubes;
using Slic3r::Tests::is_gcode_sane;
using Slic3r::Biz::Platform::IMainThreadDispatcher;
using Slic3r::App::Platform::StdMainThreadDispatcher;
using Slic3r::Biz::Platform::PlatformServices;
using Slic3r::Biz::Slicing::FDMResult;
using Slic3r::Biz::Slicing::FDMStatistics;
using Slic3r::Biz::Slicing::ProjectBedId;
using Slic3r::Domain::SelectionId;

struct StatusEvent {
    Status status;
    ProjectBedId project_bed_id;
};

bool operator==(const StatusEvent& a, const StatusEvent& b) {
    return a.status == b.status && a.project_bed_id == b.project_bed_id;
}

using StatusEvents = std::vector<StatusEvent>;

struct StatusListener : public ISlicingStatusListener {
    virtual void on_status_changed(const Status status, const ProjectBedId id) override {
        status_events.push_back(StatusEvent{status, id});
    }

    std::vector<StatusEvent> status_events;
};

void wait_for_status(
    IMainThreadDispatcher& dispatcher,
    const StatusListener& status_listener,
    const seconds timeout,
    const std::function<bool(StatusEvents)>& condition
) {
    using namespace std::chrono_literals;

    const auto start{high_resolution_clock::now()};
    while(true) {
        dispatcher.dispatch_enqueued();
        const std::vector<StatusEvent> status_events{status_listener.status_events};
        if (!status_events.empty() && condition(status_events)) {
            break;
        }
        const auto now{high_resolution_clock::now()};
        if (duration_cast<seconds>(now - start) > timeout) {
            throw std::runtime_error{"Waiting for result timed out!"};
        }
        std::this_thread::sleep_for(1ms);
    }
}

using GCodes = std::map<ProjectBedId, std::string>;

struct ResultListener : public ISlicingResultListener
{
    virtual void on_fdm_result_changed(
        std::shared_ptr<FDMResult> result, std::shared_ptr<FDMStatistics>, const ProjectBedId id
    )
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

SlicingInteractor init_slicing_interactor(IMainThreadDispatcher &dispatcher) {
    PlatformServices::instance().set_main_thread_dispatcher(&dispatcher);
    return SlicingInteractor{};
}

struct SlicingFixture {
    SlicingFixture(): slicing(init_slicing_interactor(dispatcher))  {
        slicing.on_selected_project_changed(0);
        slicing.add_status_listener(&status_listener);
        slicing.add_result_listener(&result_listener);
    }

    StdMainThreadDispatcher dispatcher{};
    std::map<ProjectBedId, Slic3r::Model> models{};
    SlicingInteractor slicing;
    StatusListener status_listener;
    ResultListener result_listener;
};

TEST_CASE_METHOD(SlicingFixture, "Slice N beds", "[slicing][slicing-interactor]")
{
    using namespace std::chrono_literals;

    const int bed_count{5};
    for (int cube_count{1}; cube_count <= bed_count; ++cube_count) {
        const auto bed_id{static_cast<SelectionId>(cube_count)};
        const ProjectBedId id{0, bed_id};
        models[id] = generate_cubes(cube_count, 5);
        slicing.update_bed(models[id], get_config(), bed_id);
    }

    slicing.slice_all();

    wait_for_status(dispatcher, status_listener, 10s, [](const StatusEvents &events){
        return bed_count == std::ranges::count_if(events, [](const StatusEvent& event){
            return event.status == Status::Finished;
        });
    });

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

TEST_CASE_METHOD(SlicingFixture, "Update stops slicing", "[slicing][slicing-interactor]") {
    using namespace std::chrono_literals;

    const ProjectBedId id{0, 0};
    Slic3r::DynamicPrintConfig config{get_config()};
    models[id] = generate_cubes(10, 5);
    slicing.update_bed(models[id], config, id.bed_id);
    slicing.slice_bed(id.bed_id);
    slicing.update_bed(models[id], config, id.bed_id);

    wait_for_status(dispatcher, status_listener, 10s, [](const StatusEvents &events){
        return events.size() > 4;
    });

    const StatusEvents expected_events{
        StatusEvent{Status::Updating, id},
        StatusEvent{Status::Modified, id},
        StatusEvent{Status::Running, id},
        StatusEvent{Status::Stopping, id},
        StatusEvent{Status::Modified, id},
    };

    CHECK_THAT(status_listener.status_events, Equals(expected_events));
}

TEST_CASE_METHOD(SlicingFixture, "Stop pops the action from queue", "[slicing][slicing-interactor]") {
    using namespace std::chrono_literals;

    const ProjectBedId id1{0, 0};
    const ProjectBedId id2{0, 1};
    Slic3r::DynamicPrintConfig config{get_config()};
    models[id1] = generate_cubes(1, 5);
    models[id2] = generate_cubes(2, 5);
    slicing.update_bed(models[id1], config, id1.bed_id);
    slicing.update_bed(models[id2], config, id2.bed_id);
    slicing.slice_all();
    slicing.stop_slicing_bed(id2.bed_id);

    wait_for_status(dispatcher, status_listener, 10s, [](const StatusEvents &events){
        return events.back().status == Status::Finished;
    });
    // Let the second bed finish slicing if the stop failed.
    std::this_thread::sleep_for(20ms);
    dispatcher.dispatch_enqueued();

    CHECK_THAT(status_listener.status_events, Contains(StatusEvents{{Status::Finished, id1}}));
    CHECK_THAT(status_listener.status_events, !Contains(StatusEvents{{Status::Stopping, id2}}));
    CHECK_THAT(status_listener.status_events, !Contains(StatusEvents{{Status::Finished, id2}}));
}

TEST_CASE_METHOD(SlicingFixture, "Stop all stops all processes", "[slicing][slicing-interactor]") {
    using namespace std::chrono_literals;

    const ProjectBedId id1{0, 0};
    const ProjectBedId id2{0, 1};
    Slic3r::DynamicPrintConfig config{get_config()};
    models[id1] = generate_cubes(1, 5);
    models[id2] = generate_cubes(2, 5);
    slicing.update_bed(models[id1], config, id1.bed_id);
    slicing.update_bed(models[id2], config, id2.bed_id);
    slicing.slice_all();

    // Let them both start.
    wait_for_status(dispatcher, status_listener, 10s, [&](const StatusEvents &events){
        const auto it1{std::ranges::find(events, StatusEvent{Status::Running, id1})};
        const auto it2{std::ranges::find(events, StatusEvent{Status::Running, id2})};
        return it1 != events.end() && it2 != events.end();
    });
    slicing.stop_all();
    wait_for_status(dispatcher, status_listener, 10s, [&](const StatusEvents &events){
        const auto it1{std::ranges::find(events, StatusEvent{Status::Stopping, id1})};
        const auto it2{std::ranges::find(events, StatusEvent{Status::Stopping, id2})};
        return it1 != events.end() && it2 != events.end();
    });

    CHECK_THAT(status_listener.status_events, Contains(StatusEvents{{Status::Stopping, id1}}));
    CHECK_THAT(status_listener.status_events, Contains(StatusEvents{{Status::Stopping, id2}}));
}

TEST_CASE_METHOD(
    SlicingFixture,
    "Remove bed is handled gracefully with non empty queues",
    "[slicing][slicing-interactor]"
)
{
    using namespace std::chrono_literals;

    const ProjectBedId id{0, 0};
    Slic3r::DynamicPrintConfig config{get_config()};
    models[id] = generate_cubes(10, 5);
    slicing.update_bed(models[id], config, id.bed_id);
    slicing.slice_bed(id.bed_id);
    slicing.remove_bed(id.bed_id);

    wait_for_status(dispatcher, status_listener, 10s, [](const StatusEvents &events){
        return events.size() == 5;
    });

    const StatusEvents expected_events{
        StatusEvent{Status::Updating, id},
        StatusEvent{Status::Modified, id},
        StatusEvent{Status::Running, id},
        StatusEvent{Status::Stopping, id},
        StatusEvent{Status::Modified, id},
    };

    CHECK_THAT(status_listener.status_events, Equals(expected_events));
}
