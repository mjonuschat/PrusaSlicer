#pragma once

#include <fmt/core.h>
#include <mutex>

#include <Slic3r/Domain/SelectionId.hpp>
#include <Slic3r/Domain/ProjectMetadata.hpp>
#include <Slic3r/Domain/ConfigContainer.hpp>
#include <Slic3r/Domain/GCodeMetadata.hpp>
#include <Slic3r/Biz/libpgcode/ProcessorResult.hpp>
#include "libslic3r/SlicingStatus.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/SlicingId.hpp"

#include "libslic3r/PrintBase.hpp"

#include <jthread/JThread.hpp>

namespace Slic3r::Biz::Slicing::Sla {
struct Object;
} // namespace Slic3r::Biz::Slicing::Sla

namespace Slic3r::Biz::Slicing {
struct SLAResult;

struct LoggingScopeLock {
    LoggingScopeLock(std::mutex &mutex, std::string id);
    ~LoggingScopeLock();

private:
    std::mutex& m_mutex;
    std::string m_id;
};

bool is_thread_active(const StatusCode status);

Domain::GCodeMetadata build_gcode_metadata(
    const Domain::ProjectMetadata& project_metadata,
    const Domain::Preset::SelectedPresetMetadata& preset_metadata,
    const Domain::ConfigPack& config
);

PrintBase::MetadataSerializeFn build_metadata_serializer(
    const Domain::GCodeMetadata& metadata,
    const Domain::Preset::SelectedPresetMetadata& preset_metadata,
    const Domain::ConfigPack& config
);

using FDMResult = libpgcode::ProcessorResult;

class IProcessCallbacks {
public:
    virtual void on_fdm_result(FDMResult &&, Domain::SlicingId) = 0;
    virtual void on_sla_result(const Domain::SlicingId&, SLAResult&&) = 0;
    virtual void on_sla_object(const Domain::SlicingId&, Sla::Object&&) = 0;
    virtual void on_status(const StatusUpdate, Domain::SlicingId) = 0;
    virtual void on_exception(std::exception_ptr exception, Domain::SlicingId) = 0;
    virtual void on_wipe_tower_geometry(Print::OptWipeTowerGeometry&&, Domain::SlicingId) = 0;
    virtual void on_extruder_candidates(std::vector<unsigned>&& extruder_candidates, Domain::SlicingId) = 0;
    virtual StatusCode get_status(const Domain::SlicingId) const = 0;
    virtual ~IProcessCallbacks() = default;
};

class BackgroundProcess
{
public:
    BackgroundProcess(
        IProcessCallbacks& callbacks,
        Domain::Model& model,
        const Domain::ProjectMetadata& project_metadata,
        const Domain::Preset::SelectedPresetMetadata& preset_metadata,
        const Domain::ConfigPack& config,
        const Domain::BedInstance& bed,
        const Domain::SlicingId id
    );
    BackgroundProcess(
        std::unique_ptr<Print::IPrint>&& print,
        IProcessCallbacks& callbacks,
        Domain::Model& model,
        const Domain::ProjectMetadata& project_metadata,
        const Domain::Preset::SelectedPresetMetadata& preset_metadata,
        const Domain::ConfigPack& config,
        const Domain::BedInstance& bed,
        const Domain::SlicingId id
    );
    ~BackgroundProcess();

    /* WARNING! It is up to the caller to ensure update is not called on a running thread! */
    void update(
        Domain::Model& model,
        const Domain::ProjectMetadata& project_metadata,
        const Domain::Preset::SelectedPresetMetadata& preset_metadata,
        const Domain::ConfigPack& config,
        const Domain::BedInstance& bed
    );

    void slice(IThumbnailImageGenerator& thumbnail_generator);
    void stop();

    std::string get_hw_printer_id() const;

private:
    std::string m_hw_config_id;
    std::unique_ptr<Print::IPrint> m_print;
    std::function<void(StatusUpdate)> m_on_status;
    std::function<void(std::exception_ptr)> m_on_exception;
    std::function<StatusCode()> m_get_status;
    Domain::SlicingId m_id;

    JThread::JThread m_thread;
    JThread::JThread m_helper_thread;

    void queue_action(const std::function<void()>& action);

    // Update and slice must not run at the same time.
    std::mutex m_mutex;
};
}; // namespace Slic3r::Biz::Slicing
