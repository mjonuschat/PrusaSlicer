#include "Slic3r/Biz/Platform/JobManager/ProgressTracker.hpp"

namespace Slic3r::Biz::Platform::JobManager {

ProgressTracker::ProgressTracker(IMainThreadDispatcher& dispatcher, std::function<void(Progress)> on_change):
    m_dispatcher{dispatcher},
    m_on_change{on_change}
{}

void ProgressTracker::set_status(const JobStatus status)
{
    if (!m_dispatcher.get()
             .dispatch_on_main_thread([on_change = m_on_change, progress = m_progress, status]() {
                 progress->status = status;
                 on_change(*progress);
             })) {
        SPDLOG_WARN("status not emitted");
    }
}

void ProgressTracker::set_status_unsafe(const JobStatus status)
{
    m_progress->status = status;
    m_on_change(*m_progress);
}

void ProgressTracker::set(Domain::Percentage percentage)
{
    if (!m_dispatcher.get().dispatch_on_main_thread(
            [on_change = m_on_change, progress = m_progress, percentage]() {
                ASSERT(progress->status == JobStatus::Started);
                ASSERT(Domain::Percentage{0} <= percentage && percentage <= Domain::Percentage{100});
                if (progress->percent) {
                    ASSERT(percentage >= progress->percent);
                }
                progress->percent = percentage;
                on_change(*progress);
            }
        )) {
        SPDLOG_WARN("progress not emitted");
    }
}

const Progress& ProgressTracker::get_progress() const
{
    return *m_progress;
}
} // namespace Slic3r::Biz::Platform::JobManager
