#include <Slic3r/Biz/Slicing/SlicingInteractor.hpp>
#include <Slic3r/Biz/Platform/PlatformServices.hpp>
#include "Slic3r/Assert.hpp"
#include "libslic3r/SLA/SLAResult.hpp"

#include <fmt/core.h>
#include <fmt/ostream.h>

namespace Slic3r::Biz::Slicing {

using Domain::Preset::SelectedPresetMetadata;
using Domain::ProjectMetadata;
using Domain::SlicingId;
using Domain::ConfigPack;

SlicingInteractor::SlicingInteractor(
    Platform::IMainThreadDispatcher& dispatcher,
    IThumbnailImageGenerator& thumbnail_image_generator
) :
    m_dispatcher(dispatcher),
    m_thumbnail_image_generator(thumbnail_image_generator)
{}

SlicingInteractor::~SlicingInteractor()
{
    ASSERT(
        m_dispatcher.is_closed(),
        "There must be no queued events (not even in the future),"
        " bacause they may remember the address of this instance!"
    );
}

void SlicingInteractor::create_process(
    Domain::Model& model,
    const ProjectMetadata& project_metadata,
    const SelectedPresetMetadata& preset_metadata,
    const ConfigPack& config,
    const Domain::BedInstance& bed,
    const SlicingId id
)
{
    SPDLOG_INFO("{}: create process", fmt::streamed(id));
    update_status(id, Status::Modified);
    m_processes.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(
            *this,
            model,
            ProjectMetadata{project_metadata},
            SelectedPresetMetadata{preset_metadata},
            ConfigPack{config},
            bed,
            id
        )
    );
}

void SlicingInteractor::update_process(
    Domain::Model& model,
    const ProjectMetadata& project_metadata,
    const SelectedPresetMetadata& preset_metadata,
    const ConfigPack& config,
    const Domain::BedInstance& bed
)
{
    const Domain::SelectionId bed_instance_id{bed.id().id};
    const SlicingId id{get_process_id(bed_instance_id)};
    if (m_processes.contains(id)) {
        SPDLOG_INFO("{}: update process", fmt::streamed(id));

        stop_slicing_bed(id);
        m_update_requests.insert_or_assign(
            id,
            UpdateRequest{model, project_metadata, preset_metadata, config, bed}
        );
        process_update_requests();
        return;
    }

    create_process(model, project_metadata, preset_metadata, config, bed, id);
}

void SlicingInteractor::remove_bed(const Domain::SelectionId bed_instance_id)
{
    const SlicingId id{get_process_id(bed_instance_id)};

    stop_slicing_bed(id);
    m_processes.erase(id);
    {
        const LoggingScopeLock lock{m_status_mutex, "slicing statuses"};
        m_statuses.erase(id);
    }
    process_slicing_queue();

    invoke_listener<IFDMResultListener>([&id](auto* listener) {
        listener->on_fdm_result_changed({}, id);
    });
    invoke_listener<ISLAResultListener>([&id](auto* listener) {
        listener->on_sla_result_changed(id, {});
    });
    invoke_listener<ISLAObjectListener>([&id](auto* listener) { listener->on_remove_bed(id); });
}

void SlicingInteractor::slice_bed(const SlicingId id)
{
    ASSERT(m_processes.contains(id));
    SPDLOG_INFO("{}: slicing request", fmt::streamed(id));

    m_slicing_queue.push_back(id);
    process_slicing_queue();
}

void SlicingInteractor::stop_slicing_bed(const Domain::SlicingId id)
{
    ASSERT(m_processes.contains(id));

    const auto it{std::ranges::find(m_slicing_queue, id)};
    if (it != m_slicing_queue.end()) {
        SPDLOG_INFO("{}: slicing_queue: remove", fmt::streamed(id));
        m_slicing_queue.erase(it);
    }

    m_processes.at(id).stop();
}

void SlicingInteractor::slice_all()
{
    m_slicing_queue = {};
    for (const auto& pair : m_processes) {
        const SlicingId id{pair.first};
        const Status status{get_status(id)};
        if (status == Status::Empty || status == Status::Finished) {
            continue;
        }
        SPDLOG_INFO("{}: slicing request", fmt::streamed(id));
        m_slicing_queue.push_back(id);
    }
    process_slicing_queue();
}

void SlicingInteractor::stop_all()
{
    m_slicing_queue = {};
    for (auto& pair : m_processes) {
        BackgroundProcess& process{pair.second};
        process.stop();
    }
}

void SlicingInteractor::on_selected_project_changed(size_t index)
{
    m_current_project_id = index;
}

SlicingId SlicingInteractor::get_process_id(const Domain::SelectionId bed_instance_id) const
{
    ASSERT(m_current_project_id != Domain::INVALID_ID);
    return {m_current_project_id, bed_instance_id};
}

void SlicingInteractor::on_status(const Status status, const SlicingId id)
{
    SPDLOG_INFO("{}: status: {}", fmt::streamed(id), fmt::streamed(status));

    {
        const LoggingScopeLock lock{m_status_mutex, "slicing statuses"};
        if (m_statuses.contains(id)) {
            m_statuses[id] = status;
        }
    }

    if (!m_dispatcher.dispatch_on_main_thread([this, status, id]() {
        process_slicing_queue();
        invoke_listeners<IStatusListener>([&](auto* listener) {
            listener->on_status_changed(status, id);
        });
    }))
    {
        SPDLOG_INFO("{}: status not dispatched", fmt::streamed(id), fmt::streamed(status));
    }
}

void SlicingInteractor::on_fdm_result(FDMResult&& result, const SlicingId id)
{
    SPDLOG_INFO("{}: FDMResult{{moves_count: {}}}", fmt::streamed(id), result.const_moves()->size());

    using Platform::MoveOnlyFunction;

    if (!m_dispatcher.dispatch_on_main_thread([this, id, _result = std::move(result)]() mutable {
        bool already_called{false};
        invoke_listener<IFDMResultListener>([&](auto* listener) {
            listener->on_fdm_result_changed(std::move(_result), id);
            already_called = true;
        });
    }))
    {
        SPDLOG_INFO("{}: fdm result not dispatched", fmt::streamed(id), result.const_moves()->size());
    }
}

void SlicingInteractor::on_sla_result(const SlicingId& id, SLAResult&& result)
{
    SPDLOG_INFO("{}: SLAResult{{}}", fmt::streamed(id));
    auto changed = [id, _result = std::move(result)](auto* listener) mutable {
        listener->on_sla_result_changed(id, std::move(_result));
    };
    auto invoke = [this, _changed = std::move(changed)]() mutable {
        invoke_listener<ISLAResultListener>(std::move(_changed));
    };
    if (!m_dispatcher.dispatch_on_main_thread(std::move(invoke))) {
        SPDLOG_INFO("{}: sla result not dispatched", fmt::streamed(id));
    }
}

void SlicingInteractor::on_sla_object(const SlicingId& id, Sla::Object&& instance)
{
    SPDLOG_INFO("{}: SLAInstance{{}}", fmt::streamed(id));
    auto changed = [id, _instance = std::move(instance)](auto* listener) mutable {
        listener->on_sla_object_changed(id, std::move(_instance));
    };
    auto invoke = [this, _changed = std::move(changed)]() mutable {
        invoke_listener<ISLAObjectListener>(std::move(_changed));
    };
    if (!m_dispatcher.dispatch_on_main_thread(std::move(invoke))) {
        SPDLOG_INFO("{}: sla instance not dispatched", fmt::streamed(id));
    }
}

void SlicingInteractor::on_wipe_tower_geometry(
    Print::WipeTowerGeometry&& wipe_tower_geometry,
    const SlicingId id
)
{
    SPDLOG_INFO("{}: WipeTowerGeometry{{size: {}}}", fmt::streamed(id), wipe_tower_geometry.size());

    if (!m_dispatcher.dispatch_on_main_thread(
            [this, id, geometry = std::move(wipe_tower_geometry)]() mutable {
        invoke_listeners<IWipeTowerGeometryListener>([&](auto* listener) {
            listener->on_wipe_tower_geometry(geometry, id);
        });
    }
        ))
    {
        SPDLOG_INFO("{}: wipe tower geometry not dispatched", fmt::streamed(id));
    }
}

void SlicingInteractor::process_slicing_queue()
{
    process_update_requests();

    if (m_slicing_queue.empty()) {
        return;
    }

    if (get_active_processes_count() >= 2) {
        return;
    }

    SPDLOG_INFO("slicing_queue: {}", m_slicing_queue.size());

    const SlicingId to_slice{m_slicing_queue.front()};
    m_slicing_queue.pop_front();

    if (!m_processes.contains(to_slice)) {
        SPDLOG_INFO("{}: removed: nonexistent", fmt::streamed(to_slice));
        return;
    }

    m_processes.at(to_slice).slice(m_thumbnail_image_generator);
}

void SlicingInteractor::process_update_requests()
{
    if (m_update_requests.empty()) {
        return;
    }

    std::erase_if(m_update_requests, [&](const auto& pair) {
        const SlicingId id{pair.first};
        return !m_processes.contains(id);
    });

    SPDLOG_INFO("update_requests: {}", m_update_requests.size());

    std::set<SlicingId> to_remove;
    for (auto& [id, request] : m_update_requests) {
        BackgroundProcess& process{m_processes.at(id)};
        if (is_thread_active(get_status(id))) {
            continue;
        }

        //const SelectedP
        const ConfigPack& config{request.config.get()};
        const Domain::BedInstance& bed{request.bed.get()};

        if (process.get_printer_technology() != get_printer_technology(config)) {
            m_processes.erase(id);
            create_process(
                request.model.get(),
                request.project_metadata.get(),
                request.preset_metadata.get(),
                config,
                bed,
                id
            );
            continue;
        }

        process.update(request.model.get(), request.project_metadata, request.preset_metadata, config, bed);
        to_remove.insert(id);
    }

    std::erase_if(m_update_requests, [&](const auto& pair) {
        const SlicingId id{pair.first};
        return to_remove.contains(id);
    });
}

void SlicingInteractor::update_status(const SlicingId id, const Status status)
{
    const LoggingScopeLock lock{m_status_mutex, "slicing statuses"};
    m_statuses[id] = status;
}

int64_t SlicingInteractor::get_active_processes_count() const
{
    const LoggingScopeLock lock{m_status_mutex, "slicing statuses"};
    return std::ranges::count_if(m_statuses, [](const auto& pair) {
        const Status status{pair.second};
        return is_thread_active(status);
    });
}

Status SlicingInteractor::get_status(const SlicingId id) const
{
    const LoggingScopeLock lock{m_status_mutex, "slicing statuses"};
    ASSERT(m_statuses.contains(id));

    return m_statuses.at(id);
}

} // namespace Slic3r::Biz::Slicing
