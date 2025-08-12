#pragma once

#include <map>
#include <boost/functional/hash.hpp>

#include <Slic3r/Domain/SelectionId.hpp>
#include <Slic3r/Biz/Platform/ListenerList.hpp>
#include <Slic3r/Biz/Platform/IMainThreadDispatcher.hpp>
#include <Slic3r/Biz/ISelectedProjectChangedListener.hpp>
#include <Slic3r/Biz/Platform/PlatformServices.hpp>
#include <Slic3r/Domain/ConfigContainer.hpp>
#include <Slic3r/Biz/Platform/WithListeners.hpp>

#include "BackgroundProcess.hpp"
#include "libslic3r/IThumbnailImageGenerator.hpp"

namespace Slic3r::Biz::Slicing::Sla {
struct Object;
} // namespace Slic3r::Biz::Slicing::Sla

namespace Slic3r::Biz::Slicing {
struct SLAResult;

class ISlicingListener
{
public:
    virtual ~ISlicingListener() = default;
};

class IFDMResultListener : public ISlicingListener
{
public:
    virtual void on_fdm_result_changed(FDMResult&&, const Domain::SlicingId) = 0;
};

class ISLAResultListener : public ISlicingListener
{
public:
    virtual void on_sla_result_changed(const Domain::SlicingId&, SLAResult&&) = 0;
};

class ISLAObjectListener : public ISlicingListener
{
public:
    virtual void on_sla_object_changed(const Domain::SlicingId&, Sla::Object&&) = 0;
    virtual void on_remove_bed(const Domain::SlicingId&)                        = 0;
};

struct IStatusListener : ISlicingListener
{
    virtual void on_status_changed(const Status, const Domain::SlicingId) = 0;
};

struct IWipeTowerGeometryListener : ISlicingListener
{
    virtual void on_wipe_tower_geometry(Print::WipeTowerGeometry, const Domain::SlicingId) = 0;
};

struct UpdateRequest
{
    std::reference_wrapper<Domain::Model> model;
    std::reference_wrapper<const Domain::ProjectMetadata> project_metadata;
    std::reference_wrapper<const Domain::Preset::SelectedPresetMetadata> preset_metadata;
    std::reference_wrapper<const Domain::ConfigPack> config;
    std::reference_wrapper<const Domain::BedInstance> bed;
};

class SlicingInteractor :
    public ISelectedProjectChangedListener,
    public IProcessCallbacks,
    public WithListeners<IStatusListener, IWipeTowerGeometryListener>,
    public WithListener<IFDMResultListener, ISLAResultListener, ISLAObjectListener>
{
public:
    SlicingInteractor(
        Platform::IMainThreadDispatcher& dispatcher,
        IThumbnailImageGenerator& thumbnail_image_generator
    );
    ~SlicingInteractor();

    /* WARNING!
     * Update won't be performed immediately if the background process is running. Rather, the
     * process will be signaled to stop and the update scheduled after the stop happens. This means
     * that model and config references must be valid as long as the process exists! After the process
     * is removed (using remove_process) the validity of the references is no longer required.*/
    void update_process(
        Domain::Model& model,
        const Domain::ProjectMetadata& project_metadata,
        const Domain::Preset::SelectedPresetMetadata& preset_metadata,
        const Domain::ConfigPack& config,
        const Domain::BedInstance& bed
    );

    /* Blocks the UI thread if the process is running! */
    void remove_bed(const Domain::SelectionId bed_instance_id);
    void slice_bed(const Domain::SlicingId slicing_id);
    void stop_slicing_bed(const Domain::SlicingId slicing_id);
    void slice_all();
    void stop_all();

    void on_selected_project_changed(size_t index) override;

    void on_fdm_result(FDMResult&&, Domain::SlicingId) override;
    void on_sla_result(const Domain::SlicingId&, SLAResult&&) override;
    void on_sla_object(const Domain::SlicingId&, Sla::Object&&) override;
    void on_status(const Status, Domain::SlicingId) override;
    void on_wipe_tower_geometry(Print::WipeTowerGeometry&& wipe_tower_geometry, const Domain::SlicingId id) override;
    Status get_status(const Domain::SlicingId id) const override;

private:
    Domain::SlicingId get_process_id(const Domain::SelectionId bed_instance_id) const;

    void process_slicing_queue();
    void process_update_requests();
    int64_t get_active_processes_count() const;
    void update_status(const Domain::SlicingId id, const Status status);
    void create_process(
        Domain::Model& model,
        const Domain::ProjectMetadata& project_metadata,
        const Domain::Preset::SelectedPresetMetadata& preset_metadata,
        const Domain::ConfigPack& config,
        const Domain::BedInstance& bed,
        const Domain::SlicingId id
    );

    // WARNING: Do not reorder, if you do not know what you are doing!
    // Any members accessed by the threads must be destroyed after
    // the threads!
    mutable std::mutex m_status_mutex;
    std::map<Domain::SlicingId, Status> m_statuses;

    std::deque<Domain::SlicingId> m_slicing_queue;
    std::map<Domain::SlicingId, UpdateRequest> m_update_requests;
    Domain::SelectionId m_current_project_id{Domain::INVALID_ID};
    Platform::IMainThreadDispatcher& m_dispatcher;
    IThumbnailImageGenerator& m_thumbnail_image_generator;

    // Keep the threads last to be destroyed first.
    std::map<Domain::SlicingId, BackgroundProcess> m_processes;
};

} // namespace Slic3r::Biz::Slicing
