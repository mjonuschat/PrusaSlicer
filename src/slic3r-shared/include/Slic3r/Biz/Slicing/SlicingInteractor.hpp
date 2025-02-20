#pragma once

#include <map>
#include <atomic>
#include <boost/functional/hash.hpp>

#include <Slic3r/Domain/SelectionId.hpp>
#include <Slic3r/Biz/Platform/ListenerList.hpp>
#include <Slic3r/Biz/Platform/IMainThreadDispatcher.hpp>
#include <Slic3r/Biz/ISelectedProjectChangedListener.hpp>
#include <Slic3r/Biz/Platform/PlatformServices.hpp>
#include <Slic3r/Domain/ConfigContainer.hpp>

#include "BackgroundProcess.hpp"

namespace Slic3r::Biz::Slicing {

class ISlicingListener {
public:
    virtual ~ISlicingListener() = default;
};

class IFDMResultListener : public ISlicingListener
{
public:
    virtual void on_fdm_result_changed(
        std::shared_ptr<FDMResult>, std::shared_ptr<FDMStatistics>, const SlicingId
    ) = 0;
};

class ISLAResultListener : public ISlicingListener
{
public:
    virtual void on_sla_result_changed(const SlicingId) = 0;
};

struct IStatusListener : ISlicingListener
{
    virtual void on_status_changed(const Status, const SlicingId) = 0;
};

struct IWipeTowerGeometryListener : ISlicingListener
{
    virtual void on_wipe_tower_geometry(Print::WipeTowerGeometry, const SlicingId) = 0;
};

struct UpdateRequest {
    std::reference_wrapper<Model> model;
    std::reference_wrapper<const DynamicPrintConfig> config;
    std::reference_wrapper<const Domain::BedInstance> bed;
};

class SlicingInteractor : public ISelectedProjectChangedListener, public IProcessCallbacks
{
public:
    SlicingInteractor(Platform::IMainThreadDispatcher& dispatcher);
    ~SlicingInteractor();

    void add_listener(ISlicingListener* listener);
    void remove_listener(ISlicingListener *listener);

    /* WARNING!
     * Update won't be performed immediately if the background process is running. Rather, the
     * process will be signaled to stop and the update scheduled after the stop happens. This means
     * that model and config references must be valid as long as the process exists! After the process
     * is removed (using remove_process) the validity of the references is no longer required.*/
    void update_process(Model& model, const DynamicPrintConfig& config, const Domain::BedInstance& bed);

    /* Blocks the UI thread if the process is running! */
    void remove_bed(const Domain::SelectionId bed_instance_id);
    void slice_bed(const Domain::SelectionId bed_instance_id);
    void stop_slicing_bed(const Domain::SelectionId bed_instance_id);
    void slice_all();
    void stop_all();

    void on_selected_project_changed(size_t index) override;

    void on_fdm_result(FDMResult &&, FDMStatistics&&, SlicingId) override;
    void on_sla_result(SlicingId) override;
    void on_status(const Status, SlicingId) override;
    void on_wipe_tower_geometry(
        Print::WipeTowerGeometry&& wipe_tower_geometry, const SlicingId id
    ) override;
    Status get_status(const SlicingId id) const override;

private:
    SlicingId get_process_id(const Domain::SelectionId bed_instance_id) const;

    void process_slicing_queue();
    void process_update_requests();
    int64_t get_active_processes_count() const;
    void update_status(const SlicingId id, const Status status);
    void create_process(
        Model& model,
        const DynamicPrintConfig& config,
        const Domain::BedInstance& bed,
        const SlicingId id
    );

    // Must be the first member to be destroyed last as member process threads access it!
    mutable std::mutex m_status_mutex;

    std::map<SlicingId, BackgroundProcess> m_processes;
    std::map<SlicingId, Status> m_statuses;
    ListenerList<IFDMResultListener> m_fdm_result_listeners;
    ListenerList<ISLAResultListener> m_sla_result_listeners;
    ListenerList<IStatusListener> m_status_listeners;
    ListenerList<IWipeTowerGeometryListener> m_wipe_tower_geometry_listeners;
    std::deque<SlicingId> m_slicing_queue;
    std::map<SlicingId, UpdateRequest> m_update_requests;
    Domain::SelectionId m_current_project_id{Domain::INVALID_ID};
    Platform::IMainThreadDispatcher &m_dispatcher;
};

} // namespace Slic3r::Biz::Slicing
