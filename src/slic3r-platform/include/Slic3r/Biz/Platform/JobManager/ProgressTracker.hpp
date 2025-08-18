#pragma once

#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"
#include "Slic3r/Domain/Percentage.hpp"
#include "Slic3r/Domain/JobStatus.hpp"
#include "Slic3r/Log.hpp" // IWYU pragma: keep

namespace Slic3r::Biz::Platform::JobManager {

struct Progress
{
    Domain::JobStatus status;
    Domain::JobProgressInfo info{Domain::JobProgressInfo::None};
    std::optional<Domain::Percentage> percent;
};

class ProgressTracker
{
public:
    ProgressTracker(IMainThreadDispatcher& dispatcher, std::function<void(Progress)> on_change);

    void set_status(const Domain::JobStatus status);
    void set_status_unsafe(const Domain::JobStatus status);
    void set(Domain::Percentage percentage);
    const Progress& get_progress() const;

private:
    // All these data must only be accessed from the main thread!
    std::shared_ptr<Progress> m_progress{std::make_shared<Progress>()};
    std::reference_wrapper<IMainThreadDispatcher> m_dispatcher;
    std::function<void(Progress)> m_on_change;
};
} // namespace Slic3r::Biz::Platform::JobManager
