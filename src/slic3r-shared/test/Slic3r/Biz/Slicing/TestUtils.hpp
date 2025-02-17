#pragma once

#include "Slic3r/App/Platform/StdMainThreadDispatcher.hpp"
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include "libslic3r/Model.hpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include "Slic3r/Domain/ConfigContainer.hpp"


namespace Slic3r::Tests {
void precise_sleep(const std::chrono::milliseconds duration);

Slic3r::Model generate_cubes(const int count, const int row_size);

double get_cubes_filament_used(const Slic3r::Model &model);

Slic3r::DynamicPrintConfig get_config();

struct ModelOnBed {
    ModelOnBed(Model&& model, DynamicPrintConfig&& config);

    static Domain::Bed bed;

    Model model;
    DynamicPrintConfig config;
    Domain::BedInstance bed_instance;
};

ModelOnBed get_cubes_model(const int count, const int row_size);

struct StatusEvent {
    Biz::Slicing::Status status;
    Biz::Slicing::SlicingId project_bed_id;
};

bool operator==(const StatusEvent& a, const StatusEvent& b);

using StatusEvents = std::vector<StatusEvent>;

struct StatusListener : public Biz::Slicing::IStatusListener {
    virtual void on_status_changed(const Biz::Slicing::Status status, const Biz::Slicing::SlicingId id) override {
        status_events.push_back(StatusEvent{status, id});
    }

    std::vector<StatusEvent> status_events;
};

[[nodiscard]] bool wait_for_status(
    Slic3r::Biz::Platform::IMainThreadDispatcher& dispatcher,
    const StatusListener& status_listener,
    const std::chrono::seconds timeout,
    const std::function<bool(StatusEvents)>& condition
);

using GCodes = std::map<Domain::SelectionId, std::string>;
struct ResultListener : public Biz::Slicing::IFDMResultListener
{
    void on_fdm_result_changed(
        std::shared_ptr<Biz::Slicing::FDMResult> result,
        std::shared_ptr<Biz::Slicing::FDMStatistics>,
        const Biz::Slicing::SlicingId id
    ) override;

    GCodes gcodes;
};

struct SlicingFixture {
    SlicingFixture();

    Slic3r::App::Platform::StdMainThreadDispatcher dispatcher{};
    Biz::Slicing::SlicingInteractor slicing;
    StatusListener status_listener;
};

}
