#include <fmt/ostream.h>
#include <nlohmann/json.hpp>
#include <Slic3r/Biz/Slicing/BackgroundProcess.hpp>
#include <Slic3r/Assert.hpp>
#include "Slic3r/Biz/Config/ConfigLegacy.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"
#include "Slic3r/Domain/GCodeMetadata.hpp"
#include "Slic3r/Biz/ProjectMetadataJson.hpp"
#include "Slic3r/Biz/Config/GCodeMetadataJson.hpp"
#include "Slic3r/Log.hpp"

#include "libslic3r/libslic3r_version.h"
#include "libslic3r/Print.hpp"
#include "libslic3r/SLAPrint.hpp"
#include "libslic3r/Utils.hpp"
#include "Slic3r/Time.hpp"

namespace {
using namespace Slic3r;
using Biz::Slicing::IProcessCallbacks;
using Domain::SlicingId;
using Biz::Slicing::FDMResult;
using Biz::Slicing::SLAResult;
using Biz::Slicing::Sla::Object;
using Biz::Print::IPrint;
using Biz::Print::WipeTowerGeometry;

std::unique_ptr<IPrint> init_print(
    const Domain::PrinterTechnology& printer_technology, 
    IProcessCallbacks& callbacks,
    const SlicingId id)
{
    std::unique_ptr<PrintBase> print;
    std::reference_wrapper<IProcessCallbacks> callbacks_ref{callbacks};
    switch (printer_technology) {
    case Domain::PrinterTechnology::FFF: {
        Print::OnFdmResult on_fdm_result = [callbacks_ref, id](FDMResult&& result) {
            callbacks_ref.get().on_fdm_result(std::move(result), id); };
        Print::OnWipeTowerGeometry on_wipe_tower_geometry = [callbacks_ref, id](WipeTowerGeometry&& geometry) {
            callbacks_ref.get().on_wipe_tower_geometry(std::move(geometry), id); };
        print = std::make_unique<Print>(on_fdm_result, on_wipe_tower_geometry);
        break;
    }
    case Domain::PrinterTechnology::SLA: {
        SLAPrint::OnSlaResult on_sla_result = [callbacks_ref, id](SLAResult&& result) {
            callbacks_ref.get().on_sla_result(id, std::move(result)); };
        SLAPrint::OnSlaObject on_sla_object = [callbacks_ref, id](const Object& object) {
            auto object_copy = object;
            callbacks_ref.get().on_sla_object(id, std::move(object_copy)); };
        print = std::make_unique<SLAPrint>(on_sla_result, on_sla_object);
        break;
    }
    default:
        UNREACHABLE("Only FFF and SLA are viable options!");
    }
    print->set_status_silent();
    return print;
}

} // namespace

namespace Slic3r::Biz::Slicing {

using Print::IPrint;
namespace ApplyStatus = Print::ApplyStatus;
using JThread::StopToken;
using JThread::JThread;
using Domain::ConfigPack;
using Domain::ConfigPackFDM;
using Domain::ConfigPackSLA;

LoggingScopeLock::LoggingScopeLock(std::mutex& mutex, std::string id)
    : m_mutex{mutex}, m_id{std::move(id)}
{
    SPDLOG_TRACE("Lock '{}' locked", m_id);
    m_mutex.lock();
}

LoggingScopeLock::~LoggingScopeLock() {
    SPDLOG_TRACE("Lock '{}' unlocked", m_id);
    m_mutex.unlock();
}

std::ostream& operator<<(std::ostream& output, const StatusCode& status_code) {
    switch(status_code) {
        case StatusCode::Empty: return output << "Empty";
        case StatusCode::Updating: return output << "Updating";
        case StatusCode::Running: return output << "Running";
        case StatusCode::Finished: return output << "Finished";
        case StatusCode::Modified: return output << "Modified";
        case StatusCode::Stopping: return output << "Stopping";
        case StatusCode::Removed: return output << "Removed";
        default: return output << "Unknown";
    }
}

std::ostream& operator<<(std::ostream& output, const Status& status) {
    output << "{code: ";
    output << status.code;
    output << ", has_error: " << !status.error.empty();
    output << ", has_warrnings: " << !status.warrnings.empty();
    output << "}";
    return output;
}

bool is_thread_active(const StatusCode status) {
    return status == StatusCode::Running
        || status == StatusCode::Stopping
        || status == StatusCode::Updating;
}

Domain::PrinterTechnology get_printer_technology(const ConfigPack& config) {
    if (std::holds_alternative<ConfigPackFDM>(config)) {
        return Domain::PrinterTechnology::FFF;
    } else if (std::holds_alternative<ConfigPackSLA>(config)) {
        return Domain::PrinterTechnology::SLA;
    } else {
        PANIC("Unexpected config type!");
    }
}

BackgroundProcess::BackgroundProcess(
    IProcessCallbacks& callbacks,
    Domain::Model& model,
    Domain::ProjectMetadata&& project_metadata,
    Domain::Preset::SelectedPresetMetadata&& preset_metadata,
    ConfigPack&& config,
    const Domain::BedInstance& bed,
    const SlicingId id
)
    : m_printer_technology{Slicing::get_printer_technology(config)}
    , m_print{init_print(m_printer_technology, callbacks, id)}
    , m_on_status{[call = std::reference_wrapper(callbacks), id](const Status status) {call.get().on_status(status, id); }}
    , m_get_status{[call = std::reference_wrapper(callbacks), id]() { return call.get().get_status(id); }}
    , m_id{id}
{
    this->update(model, std::move(project_metadata), std::move(preset_metadata), std::move(config), bed);
};

BackgroundProcess::BackgroundProcess(
    std::unique_ptr<IPrint>&& print,
    IProcessCallbacks& callbacks,
    Domain::Model& model,
    Domain::ProjectMetadata&& project_metadata,
    Domain::Preset::SelectedPresetMetadata&& preset_metadata,
    ConfigPack&& config,
    const Domain::BedInstance& bed,
    const SlicingId id
)
    : m_printer_technology{Slicing::get_printer_technology(config)}
    , m_print{std::move(print)}
    , m_on_status{[call = std::reference_wrapper(callbacks), id](const Status status) {call.get().on_status(status, id); }}
    , m_get_status{[call = std::reference_wrapper(callbacks), id]() { return call.get().get_status(id); }}
    , m_id{id}
{
    this->update(model, std::move(project_metadata), std::move(preset_metadata), std::move(config), bed);
};

BackgroundProcess::~BackgroundProcess() {
    // Stop threads before sending status.
    this->m_helper_thread = {};
    this->m_thread = {};

    m_on_status({StatusCode::Removed});
}

void BackgroundProcess::update(
    Domain::Model& model,
    const Domain::ProjectMetadata& project_metadata,
    const Domain::Preset::SelectedPresetMetadata& preset_metadata,
    const ConfigPack& config,
    const Domain::BedInstance& bed
)
{
    SPDLOG_INFO("{}: update", fmt::streamed(m_id));
    const Domain::PrinterTechnology printer_technology{Slicing::get_printer_technology(config)};
    ASSERT(printer_technology == m_printer_technology);

    const LoggingScopeLock lock{m_mutex, "background process"};

    const Status previous_status{m_get_status()};
    ASSERT(!is_thread_active(previous_status.code), "Update must be called on stopped thread!");
    std::optional<ApplyStatus::Status> apply_status;
    const ScopeGuard guard{[this, &previous_status, &apply_status]() {
        ASSERT(apply_status);

        const bool unchanged{std::holds_alternative<ApplyStatus::Unchanged>(*apply_status)};
        const bool changed{std::holds_alternative<ApplyStatus::Changed>(*apply_status)};
        const bool invalid_data{std::holds_alternative<ApplyStatus::InvalidData>(*apply_status)};

        if (this->m_print->empty()) {
            this->m_on_status({StatusCode::Empty});
        } else if (previous_status.code == StatusCode::Finished && unchanged) {
            this->m_on_status({StatusCode::Finished});
        } else if (unchanged){
            this->m_on_status({StatusCode::Modified, previous_status.error, previous_status.warrnings});
        } else if (changed) {
            const auto& changed_status{std::get<ApplyStatus::Changed>(*apply_status)};
            this->m_on_status({StatusCode::Modified, "", changed_status.warrnings});
        } else if (invalid_data) {
            const auto& invalid_data_status{std::get<ApplyStatus::InvalidData>(*apply_status)};
            this->m_on_status({StatusCode::InvalidData, invalid_data_status.error, {} });
        } else {
            PANIC("Unreachable state!");
        }
    }};

    this->m_on_status({StatusCode::Updating});

    Domain::GCodeMetadata metadata{
        .general = {
            .producer = SLIC3R_APP_NAME,
            .producer_version = SLIC3R_VERSION,
            .time = Utils::iso_ext_utc_timestamp(),
        },
        .config = {
            .printer = preset_metadata.hw_config,
            .presets = {
                .vendor = preset_metadata.hw_config.vendor_id,
                .repo_id =  preset_metadata.hw_config.repo_id,
                .version = preset_metadata.hw_config.repo_version,
                .printer =  preset_metadata.printer,
                .print =  preset_metadata.print,
                .tools = preset_metadata.tools,
                .materials = preset_metadata.materials,
            }
        },
        .presets = config,
        .project = project_metadata,
        .stats = {},
    };

    const Print::SerializedConfig serialized_config{std::visit(
        [&metadata](auto&& config) {
            return Print::SerializedConfig{
                .json = beautify_json(metadata, 2, 14),
                .ini = Biz::serialize_as_legacy_config(config)
            };
        },
        config
    )};

    apply_status = this->m_print->update(model, config, bed, serialized_config);
}

void BackgroundProcess::slice(IThumbnailImageGenerator& thumbnail_generator)
{
    SPDLOG_INFO("{}: slice", fmt::streamed(m_id));

    this->stop();
    this->queue_action([this, &thumbnail_generator]() {
        this->m_thread = {}; // Wait for join.
        ASSERT(!is_thread_active(m_get_status().code), "The thread is stopped afterwards!");

        const LoggingScopeLock lock{m_mutex, "background process"};

        if (m_get_status().code != StatusCode::Modified) {
            return;
        }
        m_on_status({StatusCode::Running});
        this->m_thread = JThread{
            [this, &thumbnail_generator](StopToken stop_token, IPrint* print) {
                print->stop_token = stop_token;

                bool finished{false};
                const ScopeGuard guard{[this, &finished]() {
                    if (finished) {
                        m_on_status({StatusCode::Finished});
                    } else {
                        m_on_status({StatusCode::Modified});
                    }
                }};

                try {
                    print->slice(m_id, thumbnail_generator);
                    finished = true;
                } catch (CanceledException&) {
                }
            },
            this->m_print.get()
        };
    });
}

void BackgroundProcess::stop()
{
    this->queue_action([this]() {
        if (m_get_status().code == StatusCode::Running) {
            SPDLOG_INFO("{}: stop", fmt::streamed(m_id));
            this->m_thread.request_stop();
            this->m_on_status({StatusCode::Stopping});
        }
    });
}

Domain::PrinterTechnology BackgroundProcess::get_printer_technology() const {
    return m_printer_technology;
}

void BackgroundProcess::queue_action(const std::function<void()>& action)
{
    this->m_helper_thread = {}; // Wait for previous action to finish.
    this->m_helper_thread = JThread{action};
}

} // namespace Slic3r::Biz::BSP
