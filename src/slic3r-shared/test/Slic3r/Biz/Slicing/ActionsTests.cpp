#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <boost/filesystem/operations.hpp>

#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include "Slic3r/Biz/Slicing/ModelUtils.hpp"
#include "Slic3r/Biz/Slicing/TestUtils.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/TestUtils/TestData.hpp"
#include "Slic3r/Biz/Slicing/GCodeUtils.hpp"

using namespace Catch;
using Catch::Matchers::Equals;
using Catch::Matchers::Contains;

using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::high_resolution_clock;
using Slic3r::Biz::Slicing::Status;
using Slic3r::Tests::get_cubes_model;
using Slic3r::Tests::ModelOnBed;
using Slic3r::Biz::Slicing::FDMResult;
using Slic3r::Biz::Slicing::SlicingId;
using Slic3r::Domain::SelectionId;
using Slic3r::Biz::Print::WipeTowerGeometry;
using Slic3r::Tests::SlicingFixture;
using Slic3r::Tests::StatusEvent;
using Slic3r::Tests::StatusEvents;
using Slic3r::Tests::ResultListener;
using Slic3r::Domain::ModelInstanceList;
using Slic3r::Biz::Slicing::with_limited_instances;
using Slic3r::Tests::is_gcode_sane;
using Slic3r::Biz::Platform::PlatformServices;
using Slic3r::Biz::Slicing::IFDMResultListener;


TEST_CASE_METHOD(SlicingFixture, "Update stops slicing", "[slicing][slicing-interactor]") {
    using namespace std::chrono_literals;

    ModelOnBed model_on_bed{get_cubes_model(10, 5)};
    const SlicingId id{0, model_on_bed.bed_instance.id().id};
    slicing.update_process(model_on_bed.model, model_on_bed.config, model_on_bed.bed_instance);
    slicing.slice_bed(id.bed_instance_id);
    slicing.update_process(model_on_bed.model, model_on_bed.config, model_on_bed.bed_instance);

    REQUIRE(wait_for_status(dispatcher, status_listener, 3s, [](const StatusEvents &events){
        return events.size() > 4;
    }));

    const StatusEvents expected_events{
        StatusEvent{Status::Updating, id},
        StatusEvent{Status::Modified, id},
        StatusEvent{Status::Running, id},
        StatusEvent{Status::Stopping, id},
        StatusEvent{Status::Modified, id},
    };

    CHECK_THAT(status_listener.status_events, Equals(expected_events));
}

TEST_CASE_METHOD(SlicingFixture, "Update respects instances on bed", "[slicing][slicing-interactor]") {
    using namespace std::chrono_literals;

    ResultListener listener;
    slicing.add_listener<IFDMResultListener>(&listener);

    ModelOnBed model_on_bed{get_cubes_model(10, 5)};

    const ModelInstanceList all_instances{model_on_bed.bed_instance.model_instances()};
    const ModelInstanceList instances_to_keep{
        all_instances[2],
        all_instances[4],
        all_instances[6],
        all_instances[7],
    };
    model_on_bed.bed_instance.model_instances() = instances_to_keep;

    const SelectionId bed_id{model_on_bed.bed_instance.id().id};

    slicing.update_process(model_on_bed.model, model_on_bed.config, model_on_bed.bed_instance);
    slicing.slice_bed(bed_id);

    REQUIRE(wait_for_status(dispatcher, status_listener, 3s, [](const StatusEvents &events){
        return events.back().status == Status::Finished;
    }));

    REQUIRE(listener.gcodes.size() == 1);
    REQUIRE(listener.gcodes.contains(bed_id));
    with_limited_instances(model_on_bed.model, instances_to_keep, [&](){
        CHECK(model_on_bed.model.objects.size() == 4);
        const auto error{is_gcode_sane(listener.gcodes[bed_id], model_on_bed.model)};
        INFO((error ? *error : ""));
        CHECK(!error);
    });

}

TEST_CASE_METHOD(SlicingFixture, "Stop pops the action from queue", "[slicing][slicing-interactor]") {
    using namespace std::chrono_literals;

    ModelOnBed model_on_bed1{get_cubes_model(1, 5)};
    ModelOnBed model_on_bed2{get_cubes_model(2, 5)};
    const SlicingId id1{0, model_on_bed1.bed_instance.id().id};
    const SlicingId id2{0, model_on_bed2.bed_instance.id().id};

    slicing.update_process(model_on_bed1.model, model_on_bed1.config, model_on_bed1.bed_instance);
    slicing.update_process(model_on_bed2.model, model_on_bed2.config, model_on_bed2.bed_instance);
    slicing.slice_all();
    slicing.stop_slicing_bed(id2.bed_instance_id);

    REQUIRE(wait_for_status(dispatcher, status_listener, 3s, [](const StatusEvents &events){
        return events.back().status == Status::Finished;
    }));
    // Let the second bed finish slicing if the stop failed.
    std::this_thread::sleep_for(20ms);
    PlatformServices::instance().main_thread_dispatcher().dispatch_enqueued();

    CHECK_THAT(status_listener.status_events, Contains(StatusEvents{{Status::Finished, id1}}));
    CHECK_THAT(status_listener.status_events, !Contains(StatusEvents{{Status::Stopping, id2}}));
    CHECK_THAT(status_listener.status_events, !Contains(StatusEvents{{Status::Finished, id2}}));
}

TEST_CASE_METHOD(SlicingFixture, "Stop all stops all processes", "[slicing][slicing-interactor]") {
    using namespace std::chrono_literals;

    ModelOnBed model_on_bed1{get_cubes_model(1, 5)};
    ModelOnBed model_on_bed2{get_cubes_model(2, 5)};
    const SlicingId id1{0, model_on_bed1.bed_instance.id().id};
    const SlicingId id2{0, model_on_bed2.bed_instance.id().id};

    slicing.update_process(model_on_bed1.model, model_on_bed1.config, model_on_bed1.bed_instance);
    slicing.update_process(model_on_bed2.model, model_on_bed2.config, model_on_bed2.bed_instance);
    slicing.slice_all();

    // Let them both start.
    REQUIRE(wait_for_status(dispatcher, status_listener, 3s, [&](const StatusEvents &events){
        const auto it1{std::ranges::find(events, StatusEvent{Status::Running, id1})};
        const auto it2{std::ranges::find(events, StatusEvent{Status::Running, id2})};
        return it1 != events.end() && it2 != events.end();
    }));
    slicing.stop_all();
    REQUIRE(wait_for_status(dispatcher, status_listener, 3s, [&](const StatusEvents &events){
        const auto it1{std::ranges::find(events, StatusEvent{Status::Stopping, id1})};
        const auto it2{std::ranges::find(events, StatusEvent{Status::Stopping, id2})};
        return it1 != events.end() && it2 != events.end();
    }));

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

    ModelOnBed model_on_bed{get_cubes_model(10, 5)};
    const SlicingId id{0, model_on_bed.bed_instance.id().id};
    slicing.update_process(model_on_bed.model, model_on_bed.config, model_on_bed.bed_instance);
    slicing.slice_bed(id.bed_instance_id);
    slicing.remove_bed(id.bed_instance_id);

    REQUIRE(wait_for_status(dispatcher, status_listener, 3s, [](const StatusEvents &events){
        return events.size() == 5;
    }));

    const StatusEvents expected_events{
        StatusEvent{Status::Updating, id},
        StatusEvent{Status::Modified, id},
        StatusEvent{Status::Running, id},
        StatusEvent{Status::Stopping, id},
        StatusEvent{Status::Modified, id},
    };

    CHECK_THAT(status_listener.status_events, Equals(expected_events));
}
