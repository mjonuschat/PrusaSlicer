#include <spdlog/spdlog.h>
#include <fmt/ostream.h>
#include <Slic3r/Biz/Slicing/BackgroundProcess.hpp>
#include <Slic3r/Biz/Slicing/ModelUtils.hpp>
#include <libassert/assert.hpp>

namespace {
using namespace Slic3r;
using Biz::Slicing::IProcessCallbacks;
using Biz::Slicing::SlicingId;
using Biz::Slicing::FDMResult;
using Biz::Print::IPrint;
using Biz::Print::WipeTowerGeometry;
std::unique_ptr<IPrint> init_print(
    const PrinterTechnology& printer_technology, 
    IProcessCallbacks& callbacks,
    const SlicingId id)
{
    std::unique_ptr<PrintBase> print;
    std::reference_wrapper<IProcessCallbacks> callbacks_ref{callbacks};
    switch (printer_technology) {
    case ptFFF: {
        Print::OnFdmResult on_fdm_result = [callbacks_ref, id](FDMResult&& result) {
            callbacks_ref.get().on_fdm_result(std::move(result), id);
        };
        Print::OnWipeTowerGeometry on_wipe_tower_geometry = [callbacks_ref, id](WipeTowerGeometry&& geometry) {
            callbacks_ref.get().on_wipe_tower_geometry(std::move(geometry), id); };
        print = std::make_unique<Print>(on_fdm_result, on_wipe_tower_geometry);
        break;
    }
    case ptSLA:
        // TODO: Implement SLA callbacks
        print = std::make_unique<SLAPrint>();
        break;
    // case ptSLA: print = std::make_unique<Slic3r::SLAPrint>(callbacks); break;
    default:
        UNREACHABLE("Only FFF and SLA are viable options!");
    }
    print->set_status_silent();
    return print;
}

} // namespace

namespace Slic3r::Biz::Slicing {

using Print::IPrint;
using Print::ApplyStatus;
using JThread::StopToken;
using JThread::JThread;

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

std::ostream& operator<<(std::ostream& output, const SlicingId& id) {
    return output
        << "{project_id: " << id.project_id
        << ", bed_id: " << id.bed_instance_id << "}";
}

std::ostream& operator<<(std::ostream& output, const Status& status) {
    switch(status) {
        case Status::Empty: return output << "Empty";
        case Status::Updating: return output << "Updating";
        case Status::Running: return output << "Running";
        case Status::Finished: return output << "Finished";
        case Status::Modified: return output << "Modified";
        case Status::Stopping: return output << "Stopping";
        case Status::Removed: return output << "Removed";
        default: return output << "Unknown";
    }
}

bool is_thread_active(const Status status) {
    return status == Status::Running
        || status == Status::Stopping
        || status == Status::Updating;
}

Slic3r::PrinterTechnology get_printer_technology(const DynamicPrintConfig& config) {
    const auto option = config.option<ConfigOptionEnum<PrinterTechnology>>("printer_technology");
    ASSERT(option != nullptr);
    return option->value;
}

BackgroundProcess::BackgroundProcess(
    IProcessCallbacks& callbacks,
    Model& model,
    DynamicPrintConfig&& config,
    const Domain::ModelInstanceList& bed_instances,
    const SlicingId id
)
    : m_printer_technology{Slicing::get_printer_technology(config)}
    , m_print{init_print(m_printer_technology, callbacks, id)}
    , m_on_status{[call = std::reference_wrapper(callbacks), id](const Status status) {call.get().on_status(status, id); }}
    , m_get_status{[call = std::reference_wrapper(callbacks), id]() { return call.get().get_status(id); }}
    , m_id{id}
{
    this->update(model, std::move(config), bed_instances);
};

BackgroundProcess::BackgroundProcess(
    std::unique_ptr<IPrint>&& print,
    IProcessCallbacks& callbacks,
    Model& model,
    DynamicPrintConfig&& config,
    const Domain::ModelInstanceList& bed_instances,
    const SlicingId id
)
    : m_printer_technology{Slicing::get_printer_technology(config)}
    , m_print{std::move(print)}
    , m_on_status{[call = std::reference_wrapper(callbacks), id](const Status status) {call.get().on_status(status, id); }}
    , m_get_status{[call = std::reference_wrapper(callbacks), id]() { return call.get().get_status(id); }}
    , m_id{id}
{
    this->update(model, std::move(config), bed_instances);
};

BackgroundProcess::~BackgroundProcess() {
    // Stop threads before sending status.
    this->m_helper_thread = {};
    this->m_thread = {};

    m_on_status(Status::Removed);
}

void BackgroundProcess::update(
    Model& model,
    DynamicPrintConfig&& config,
    const Domain::ModelInstanceList& bed_instances
)
{
    SPDLOG_INFO("{}: update", fmt::streamed(m_id));
    const PrinterTechnology printer_technology{Slicing::get_printer_technology(config)};
    ASSERT(printer_technology == m_printer_technology);

    const LoggingScopeLock lock{m_mutex, "background process"};

    const Status previous_status{m_get_status()};
    ASSERT(!is_thread_active(previous_status), "Update must be called on stopped thread!");
    std::optional<ApplyStatus> apply_status;
    const ScopeGuard guard{[this, &previous_status, &apply_status]() {
        if (this->m_print->empty()) {
            this->m_on_status(Status::Empty);
        } else if (previous_status == Status::Finished && apply_status == ApplyStatus::unchanged) {
            this->m_on_status(Status::Finished);
        } else {
            this->m_on_status(Status::Modified);
        }
    }};

    this->m_on_status(Status::Updating);
    with_limited_instances(model, bed_instances, [&](){
        apply_status = this->m_print->update(model, std::move(config));
    });
}

void BackgroundProcess::slice()
{
    SPDLOG_INFO("{}: slice", fmt::streamed(m_id));

    this->stop();
    this->queue_action([this]() {
        this->m_thread = {}; // Wait for join.
        ASSERT(!is_thread_active(m_get_status()), "The thread is stopped afterwards!");

        const LoggingScopeLock lock{m_mutex, "background process"};

        if (m_get_status() == Status::Empty) {
            return;
        }
        m_on_status(Status::Running);
        this->m_thread = JThread{
            [this](StopToken stop_token, IPrint* print) {
                print->stop_token = stop_token;

                bool finished{false};
                const ScopeGuard guard{[this, &finished]() {
                    if (finished) {
                        m_on_status(Status::Finished);
                    } else {
                        m_on_status(Status::Modified);
                    }
                }};

                try {
                    print->slice();
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
        if (m_get_status() == Status::Running) {
            SPDLOG_INFO("{}: stop", fmt::streamed(m_id));
            this->m_thread.request_stop();
            this->m_on_status(Status::Stopping);
        }
    });
}

Slic3r::PrinterTechnology BackgroundProcess::get_printer_technology() const {
    return m_printer_technology;
}

void BackgroundProcess::queue_action(const std::function<void()>& action)
{
    this->m_helper_thread = {}; // Wait for previous action to finish.
    this->m_helper_thread = JThread{action};
}

} // namespace Slic3r::Biz::BSP
