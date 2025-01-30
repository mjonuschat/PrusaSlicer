#include <spdlog/spdlog.h>
#include <fmt/ostream.h>
#include <Slic3r/Biz/Slicing/BackgroundProcess.hpp>
#include <libassert/assert.hpp>

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
    m_mutex.unlock();
    SPDLOG_TRACE("Lock '{}' unlocked", m_id);
}

std::ostream& operator<<(std::ostream& output, const ProjectBedId& project_bed_id) {
    return output
        << "{project_id: " << project_bed_id.project_id
        << ", bed_id: " << project_bed_id.bed_id << "}";
}

std::ostream& operator<<(std::ostream& output, const Status& status) {
    switch(status) {
        case Status::Empty: return output << "Empty";
        case Status::Updating: return output << "Updating";
        case Status::Running: return output << "Running";
        case Status::Finished: return output << "Finished";
        case Status::Modified: return output << "Modified";
        case Status::Stopping: return output << "Stopping";
        default: return output << "Unknown";
    }
}

bool is_thread_active(const Status status) {
    return status == Status::Running
        || status == Status::Stopping
        || status == Status::Updating;
}

std::unique_ptr<Slic3r::Print> init_print() {
    auto print{std::make_unique<Slic3r::Print>()};
    print->set_status_silent();
    return print;
}

BackgroundProcess::BackgroundProcess(
    IProcessCallbacks& callbacks,
    const Model& model,
    DynamicPrintConfig&& config,
    const ProjectBedId project_bed_id
)
    : m_print{init_print()}, m_callbacks{callbacks}, m_project_bed_id{project_bed_id}
{
    this->update(model, std::move(config));
};

BackgroundProcess::BackgroundProcess(
    std::unique_ptr<IPrint>&& print,
    IProcessCallbacks& callbacks,
    const Model& model,
    DynamicPrintConfig&& config,
    const ProjectBedId project_bed_id
)
    : m_print{std::move(print)}, m_callbacks{callbacks}, m_project_bed_id{project_bed_id}
{
    this->update(model, std::move(config));
};

BackgroundProcess::~BackgroundProcess() = default;

void BackgroundProcess::update(const Model& model, DynamicPrintConfig&& config)
{
    SPDLOG_INFO("{}: update", fmt::streamed(m_project_bed_id));

    const LoggingScopeLock lock{m_mutex, "background process"};

    const Status previous_status{m_callbacks.get_status(m_project_bed_id)};
    ASSERT(!is_thread_active(previous_status), "Update must be called on stopped thread!");
    std::optional<ApplyStatus> apply_status;
    const ScopeGuard guard{[this, &previous_status, &apply_status]() {
        if (this->m_print->empty()) {
            this->on_status(Status::Empty);
        } else if (previous_status == Status::Finished && apply_status == ApplyStatus::unchanged) {
            this->on_status(Status::Finished);
        } else {
            this->on_status(Status::Modified);
        }
    }};

    this->on_status(Status::Updating);
    apply_status = this->m_print->update(model, std::move(config));
}

void BackgroundProcess::slice()
{
    SPDLOG_INFO("{}: slice", fmt::streamed(m_project_bed_id));

    this->stop();
    this->queue_action([this]() {
        this->m_thread = {}; // Wait for join.
        ASSERT(!is_thread_active(m_callbacks.get_status(m_project_bed_id)), "The thread is stopped afterwards!");

        const LoggingScopeLock lock{m_mutex, "background process"};

        if (m_callbacks.get_status(m_project_bed_id) == Status::Empty) {
            return;
        }
        on_status(Status::Running);
        this->m_thread = JThread{
            [this](StopToken stop_token, IPrint* print) {
                print->stop_token = stop_token;
                print->on_result =
                    [this](GCodeProcessorResult&& result, PrintStatistics&& print_statistics) {
                        this->m_callbacks.on_fdm_result(std::move(result), std::move(print_statistics), m_project_bed_id);
                    };

                bool finished{false};
                const ScopeGuard guard{[this, &finished]() {
                    if (finished) {
                        on_status(Status::Finished);
                    } else {
                        this->on_status(Status::Modified);
                    }
                }};

                try {
                    print->slice();
                    finished = true;
                } catch (CanceledException&) {
                }
            },
            this->m_print.get()};
    });
}

void BackgroundProcess::stop()
{
    this->queue_action([this]() {
        if (m_callbacks.get_status(m_project_bed_id) == Status::Running) {
            SPDLOG_INFO("{}: stop", fmt::streamed(m_project_bed_id));
            this->m_thread.request_stop();
            this->on_status(Status::Stopping);
        }
    });
}

void BackgroundProcess::queue_action(const std::function<void()>& action)
{
    this->m_helper_thread = {}; // Wait for previous action to finish.
    this->m_helper_thread = JThread{action};
}

void BackgroundProcess::on_status(const Status status) {
    m_callbacks.on_status(status, m_project_bed_id);
}

} // namespace Slic3r::Biz::BSP
