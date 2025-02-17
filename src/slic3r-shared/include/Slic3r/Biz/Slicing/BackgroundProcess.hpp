#pragma once

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <mutex>

#include <Slic3r/Domain/SelectionId.hpp>
#include <Slic3r/Domain/ConfigContainer.hpp>

#include <libslic3r/Print.hpp>
#include <libslic3r/GCode.hpp>
#include <libslic3r/SLAPrint.hpp>
#include <libslic3r/TriangleMesh.hpp>
#include <libslic3r/Model.hpp>

#include <jthread/JThread.hpp>

/*
struct SlicingProcessOutput {
    std::optional<Polygons> clearance_contours;

    // GUI::GLCanvas3D does some things with print->objects to determine
    // the state of the following. All these values are per object.
    std::shared_ptr<const TriangleMesh> sla_backend_mesh;
    std::shared_ptr<const TriangleMesh> sla_supports_mesh;
    std::shared_ptr<const TriangleMesh> sla_pad_mesh;
    std::shared_ptr<const std::vector<sla::SupportPoint>> points;
    double sla_print_z;

    std::shared_ptr<const WipeTowerData> wipe_tower_data;
    std::shared_ptr<const GCodeProcessorResult> gcode_processor_result;
    std::optional<SLAPrintStatistics> sla_print_statistics;
    std::optional<PrintStatistics> print_statistics; //maybe should be part of GCodeProcessorResult?

    std::optional<ToolOrdering> tool_ordering; // TickCodesManager needs that for_get_used_extruders
};
*/

namespace Slic3r::Biz::Slicing {

struct LoggingScopeLock {
    LoggingScopeLock(std::mutex &mutex, std::string id);
    ~LoggingScopeLock();

private:
    std::mutex& m_mutex;
    std::string m_id;
};

struct SlicingId {
    Domain::SelectionId project_id{};
    Domain::SelectionId bed_instance_id{};

    bool operator<(const SlicingId& other) const {
        if (project_id != other.project_id) {
            return project_id < other.project_id;
        }
        return bed_instance_id < other.bed_instance_id;
    }

    bool operator==(const SlicingId& b) const {
        return bed_instance_id == b.bed_instance_id && project_id == b.project_id;
    }
};

std::ostream& operator<<(std::ostream& output, const SlicingId& id);

enum class Status
{
    Empty,
    Updating,
    Running,
    Finished,
    Modified,
    Stopping,
};

std::ostream& operator<<(std::ostream& output, const Status& status);

bool is_thread_active(const Status status);

Slic3r::PrinterTechnology get_printer_technology(const DynamicPrintConfig& config);

using FDMResult = GCodeProcessorResult;

using FDMStatistics = PrintStatistics;

class IProcessCallbacks {
public:
    virtual void on_fdm_result(FDMResult &&, FDMStatistics&&, SlicingId) = 0;
    virtual void on_sla_result(SlicingId) = 0;
    virtual void on_status(const Status, SlicingId) = 0;
    virtual void on_wipe_tower_geometry(Print::WipeTowerGeometry&&, SlicingId) = 0;
    virtual Status get_status(const SlicingId) const = 0;
    virtual ~IProcessCallbacks() = default;
};

class BackgroundProcess
{
public:
    BackgroundProcess(
        IProcessCallbacks& callbacks,
        Model& model,
        DynamicPrintConfig&& config,
        const Domain::ModelInstanceList& bed_instances,
        const SlicingId id
    );
    BackgroundProcess(
        std::unique_ptr<Print::IPrint>&& print,
        IProcessCallbacks& callbacks,
        Model& model,
        DynamicPrintConfig&& config,
        const Domain::ModelInstanceList& bed_instances,
        const SlicingId id
    );
    ~BackgroundProcess();

    /* WARNING! It is up to the caller to ensure update is not called on a running thread! */
    void update(
        Model& model,
        DynamicPrintConfig&& config,
        const Domain::ModelInstanceList& bed_instances
    );

    void slice();
    void stop();

    Slic3r::PrinterTechnology get_printer_technology() const;

private:
    Slic3r::PrinterTechnology m_printer_technology;
    std::unique_ptr<Print::IPrint> m_print;
    IProcessCallbacks& m_callbacks;
    SlicingId m_id;

    std::function<Status()> m_get_status;
    JThread::JThread m_thread;
    JThread::JThread m_helper_thread;

    void queue_action(const std::function<void()>& action);
    void on_status(const Status status);
    void hook_callbacks(Print::IPrint* print);

    // Update and slice must not run at the same time.
    std::mutex m_mutex;
};
}; // namespace Slic3r::Biz::Slicing
