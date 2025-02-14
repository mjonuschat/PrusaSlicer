#pragma once

#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include "libslic3r/Model.hpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>


namespace Slic3r::Tests {
void precise_sleep(const std::chrono::milliseconds duration);

Slic3r::Model generate_cubes(const int count, const int row_size);

double get_cubes_filament_used(const Slic3r::Model &model);

Slic3r::DynamicPrintConfig get_config();

struct StatusEvent {
    Biz::Slicing::Status status;
    Biz::Slicing::ProjectBedId project_bed_id;
};

bool operator==(const StatusEvent& a, const StatusEvent& b);

using StatusEvents = std::vector<StatusEvent>;

struct StatusListener : public Biz::Slicing::ISlicingStatusListener {
    virtual void on_status_changed(const Biz::Slicing::Status status, const Biz::Slicing::ProjectBedId id) override {
        status_events.push_back(StatusEvent{status, id});
    }

    std::vector<StatusEvent> status_events;
};

void wait_for_status(
    Slic3r::Biz::Platform::IMainThreadDispatcher& dispatcher,
    const StatusListener& status_listener,
    const std::chrono::seconds timeout,
    const std::function<bool(StatusEvents)>& condition
);


struct SlicingFixture {
    SlicingFixture();

    Slic3r::App::Platform::StdMainThreadDispatcher dispatcher{};
    std::map<Biz::Slicing::ProjectBedId, Slic3r::Model> models{};
    Biz::Slicing::SlicingInteractor slicing;
    StatusListener status_listener;
};

}
