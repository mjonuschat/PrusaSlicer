#pragma once

#include <map>
#include <atomic>
#include <boost/functional/hash.hpp>

#include <Slic3r/Domain/SelectionId.hpp>
#include <Slic3r/Biz/Platform/ListenerList.hpp>
#include <Slic3r/Biz/Platform/IMainThreadDispatcher.hpp>
#include <Slic3r/Biz/ISelectedProjectChangedListener.hpp>
#include <Slic3r/Biz/Platform/PlatformServices.hpp>

#include "BackgroundProcess.hpp"

namespace Slic3r::Biz::Slicing {

class ISlicingResultListener
{
public:
    virtual void on_fdm_result_changed(
        std::shared_ptr<FDMResult>, std::shared_ptr<FDMStatistics>, const ProjectBedId
    ) = 0;
    virtual ~ISlicingResultListener() = default;
};

struct ISlicingStatusListener
{
    virtual void on_status_changed(const Status, const ProjectBedId) = 0;
    virtual ~ISlicingStatusListener() = default;
};

struct ISlicingWipeTowerGeometryListener
{
    virtual void on_wipe_tower_geometry(Print::WipeTowerGeometry, const ProjectBedId) = 0;
    virtual ~ISlicingWipeTowerGeometryListener() = default;
};

struct UpdateRequest {
    std::reference_wrapper<const Slic3r::Model> model;
    std::reference_wrapper<const DynamicPrintConfig> config;
};

class SlicingInteractor : public ISelectedProjectChangedListener, public IProcessCallbacks
{
public:
    void add_result_listener(ISlicingResultListener* listener);
    void remove_result_listener(ISlicingResultListener *listener);
    void add_status_listener(ISlicingStatusListener* listener);
    void remove_status_listener(ISlicingStatusListener *listener);
    void add_wipe_tower_geometry_listener(ISlicingWipeTowerGeometryListener *listener);
    void remove_wipe_tower_geometry_listener(ISlicingWipeTowerGeometryListener *listener);

    /* WARNING!
     * Update won't be performed immediately if the background process is running. Rather, the
     * process will be signaled to stop and the update scheduled after the stop happens. This means
     * that model and config references must be valid as long as the process exists! After the process
     * is removed (using remove_bed) the validity of the references is no longer required.*/
    void update_bed(const Model& model, const DynamicPrintConfig& config, const Domain::SelectionId bed);

    /* Blocks the UI thread if there is a running process on the bed! */
    void remove_bed(const Domain::SelectionId bed);
    void slice_bed(const Domain::SelectionId bed);
    void stop_slicing_bed(const Domain::SelectionId bed);
    void slice_all();
    void stop_all();

    void on_selected_project_changed(size_t index) override;

    void on_fdm_result(FDMResult &&, FDMStatistics&&, ProjectBedId) override;
    void on_status(const Status, ProjectBedId) override;
    void on_wipe_tower_geometry(
        Print::WipeTowerGeometry&& wipe_tower_geometry, const ProjectBedId project_bed_id
    ) override;
    Status get_status(const ProjectBedId project_bed_id) const override;

private:
    ProjectBedId get_project_bed_id(const Domain::SelectionId bed_id) const;

    void process_slicing_queue();
    void process_update_requests();
    int64_t get_active_processes_count() const;
    void update_status(const ProjectBedId project_bed_id, const Status status);

    std::map<ProjectBedId, BackgroundProcess> m_processes;
    std::map<ProjectBedId, Status> m_statuses;
    ListenerList<ISlicingResultListener> m_result_listeners;
    ListenerList<ISlicingStatusListener> m_status_listeners;
    ListenerList<ISlicingWipeTowerGeometryListener> m_wipe_tower_geometry_listeners;
    std::deque<ProjectBedId> m_slicing_queue;
    std::map<ProjectBedId, UpdateRequest> m_update_requests;
    Domain::SelectionId m_current_project_id{Domain::INVALID_ID};
    Platform::IMainThreadDispatcher &m_dispatcher{Platform::PlatformServices::instance().main_thread_dispatcher()};
    mutable std::mutex m_status_mutex;
};

} // namespace Slic3r::Biz::Slicing
