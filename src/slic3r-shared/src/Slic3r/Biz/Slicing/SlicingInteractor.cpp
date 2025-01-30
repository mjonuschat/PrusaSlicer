#include <Slic3r/Biz/Slicing/SlicingInteractor.hpp>
#include <Slic3r/Biz/Platform/PlatformServices.hpp>
#include <libassert/assert.hpp>
#include <spdlog/spdlog.h>
#include <fmt/core.h>
#include <fmt/ostream.h>

namespace Slic3r::Biz::Slicing {

void SlicingInteractor::add_result_listener(ISlicingResultListener* listener) {
    m_result_listeners.add(listener);
}

void SlicingInteractor::remove_result_listener(ISlicingResultListener *listener) {
    m_result_listeners.remove(listener);
}

void SlicingInteractor::add_status_listener(ISlicingStatusListener* listener) {
    m_status_listeners.add(listener);
}

void SlicingInteractor::remove_status_listener(ISlicingStatusListener *listener) {
    m_status_listeners.remove(listener);
}

void SlicingInteractor::update_bed(
    const Model& model,
    const DynamicPrintConfig& config,
    const Domain::SelectionId bed_id
)
{
    const ProjectBedId id{get_project_bed_id(bed_id)};
    if (m_processes.contains(id)) {
        SPDLOG_INFO("{}: update process", fmt::streamed(id));

        stop_slicing_bed(bed_id);
        m_update_requests.insert_or_assign(id, UpdateRequest{model, config});
        process_update_requests();
        return;
    }

    SPDLOG_INFO("{}: create process", fmt::streamed(id));

    update_status(id, Status::Modified);
    m_processes.emplace(
        std::piecewise_construct, std::forward_as_tuple(id),
        std::forward_as_tuple(*this, model, DynamicPrintConfig{config}, id)
    );

}

void SlicingInteractor::remove_bed(const Domain::SelectionId bed_id) {
    const ProjectBedId id{get_project_bed_id(bed_id)};

    stop_slicing_bed(bed_id);
    m_processes.erase(id);
    {
        const LoggingScopeLock lock{m_status_mutex, "slicing statuses"};
        m_statuses.erase(id);
    }
    process_slicing_queue();
}

void SlicingInteractor::slice_bed(const Domain::SelectionId bed_id) {
    const ProjectBedId id{get_project_bed_id(bed_id)};
    ASSERT(m_processes.contains(id));
    SPDLOG_INFO("{}: slicing request", fmt::streamed(id));
    m_slicing_queue.push_back(id);
    process_slicing_queue();
}

void SlicingInteractor::stop_slicing_bed(const Domain::SelectionId bed_id) {
    const ProjectBedId id{get_project_bed_id(bed_id)};
    ASSERT(m_processes.contains(id));

    const auto it{std::ranges::find(m_slicing_queue, id)};
    if (it != m_slicing_queue.end()) {
        SPDLOG_INFO("{}: slicing_queue: remove", fmt::streamed(id));
        m_slicing_queue.erase(it);
    }

    m_processes.at(id).stop();
}

void SlicingInteractor::slice_all() {
    m_slicing_queue = {};
    for (const auto& pair : m_processes) {
        const ProjectBedId id{pair.first};
        const Status status{get_status(id)};
        if (status == Status::Empty || status == Status::Finished) {
            continue;
        }
        SPDLOG_INFO("{}: slicing request", fmt::streamed(id));
        m_slicing_queue.push_back(id);
    }
    process_slicing_queue();
}

void SlicingInteractor::stop_all() {
    m_slicing_queue = {};
    for (auto& pair : m_processes) {
        BackgroundProcess &process{pair.second};
        process.stop();
    }
}


void SlicingInteractor::on_selected_project_changed(size_t index) {
    m_current_project_id = index;
}

ProjectBedId SlicingInteractor::get_project_bed_id(const Domain::SelectionId bed_id) const {
    ASSERT(m_current_project_id != Domain::INVALID_ID);
    return {m_current_project_id, bed_id};
}

void SlicingInteractor::on_status(const Status status, const ProjectBedId project_bed_id) {
    SPDLOG_INFO("{}: status: {}", fmt::streamed(project_bed_id), fmt::streamed(status));

    {
        const LoggingScopeLock lock{m_status_mutex, "slicing statuses"};
        if (m_statuses.contains(project_bed_id)) {
            m_statuses[project_bed_id] = status;
        }
    }

    m_dispatcher.dispatch_on_main_thread([=]() mutable {
        process_slicing_queue();
        m_status_listeners.invoke([&](ISlicingStatusListener* listener){
            listener->on_status_changed(status, project_bed_id);
        });
    });
}

void SlicingInteractor::on_fdm_result(FDMResult&& result, FDMStatistics&& statistics, const ProjectBedId project_bed_id) {
    SPDLOG_INFO("{}: FDMResult{{moves_count: {}}}", fmt::streamed(project_bed_id), result.moves.size());

    m_dispatcher.dispatch_on_main_thread(
        [
            this,
            project_bed_id,
            _result = std::make_shared<FDMResult>(std::move(result)),
            _statistics = std::make_shared<FDMStatistics>(std::move(statistics))
        ]() mutable {
            m_result_listeners.invoke([&](ISlicingResultListener* listener){
                listener->on_fdm_result_changed(_result, _statistics, project_bed_id);
            });
            _result.reset();
            _statistics.reset();
        }
    );
}

void SlicingInteractor::process_slicing_queue() {
    process_update_requests();

    if(m_slicing_queue.empty()) {
        return;
    }

    if (get_active_processes_count() >= 2) {
        return;
    }

    SPDLOG_INFO("slicing_queue: {}", m_slicing_queue.size());

    const ProjectBedId to_slice{m_slicing_queue.front()};
    m_slicing_queue.pop_front();

    if (!m_processes.contains(to_slice)) {
        SPDLOG_INFO("{}: removed: nonexistent", fmt::streamed(to_slice));
        return;
    }

    m_processes.at(to_slice).slice();
}

void SlicingInteractor::process_update_requests() {
    if (m_update_requests.empty()) {
        return;
    }

    std::erase_if(m_update_requests, [&](const auto& pair) {
        const ProjectBedId bed_id{pair.first};
        return !m_processes.contains(bed_id);
    });

    SPDLOG_INFO("update_requests: {}", m_update_requests.size());

    std::set<ProjectBedId> to_remove;
    for (auto& [id, request] : m_update_requests) {
        BackgroundProcess& process{m_processes.at(id)};
        if (!is_thread_active(get_status(id))) {
            process.update(request.model.get(), DynamicPrintConfig{request.config.get()});
            to_remove.insert(id);
        }
    }

    std::erase_if(m_update_requests, [&](const auto& pair) {
        const ProjectBedId id{pair.first};
        return to_remove.contains(id);
    });
}

void SlicingInteractor::update_status(const ProjectBedId project_bed_id, const Status status) {
    const LoggingScopeLock lock{m_status_mutex, "slicing statuses"};
    m_statuses[project_bed_id] = status;
}

int64_t SlicingInteractor::get_active_processes_count() const {
    const LoggingScopeLock lock{m_status_mutex, "slicing statuses"};
    return std::ranges::count_if(m_statuses, [](const auto& pair){
        const Status status{pair.second};
        return is_thread_active(status);
    });
}

Status SlicingInteractor::get_status(const ProjectBedId project_bed_id) const {
    const LoggingScopeLock lock{m_status_mutex, "slicing statuses"};
    ASSERT(m_statuses.contains(project_bed_id));

    return m_statuses.at(project_bed_id);
}

} // namespace Slic3r::Biz::Slicing
