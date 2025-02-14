#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <boost/filesystem/operations.hpp>

#include "Slic3r/Biz/Slicing/TestUtils.hpp"
#include "Slic3r/TestUtils/TestData.hpp"

using namespace Catch;
using Catch::Matchers::Equals;
using Catch::Matchers::Contains;

using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::high_resolution_clock;
using Slic3r::Biz::Slicing::Status;
using Slic3r::Tests::get_config;
using Slic3r::Tests::generate_cubes;
using Slic3r::Biz::Slicing::FDMResult;
using Slic3r::Biz::Slicing::FDMStatistics;
using Slic3r::Biz::Slicing::ProjectBedId;
using Slic3r::Domain::SelectionId;
using Slic3r::Biz::Print::WipeTowerGeometry;
using GCodes = std::map<ProjectBedId, std::string>;
using Slic3r::Tests::SlicingFixture;
using Slic3r::Tests::StatusEvent;
using Slic3r::Tests::StatusEvents;

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
