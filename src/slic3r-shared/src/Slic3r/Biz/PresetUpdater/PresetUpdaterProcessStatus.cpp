#include "PresetUpdaterProcessStatus.hpp"

#include <chrono>
#include <fmt/format.h>

using namespace std::chrono_literals;

namespace Slic3r::Biz::PresetUpdater {

const std::map<PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy, Network::HttpRetryOpt>
    PresetUpdaterProcessStatus::policy_map = {
        {PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_5_TRIES, {500ms, 5s, 4}},
        {PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_NO_RETRY, {0ms}}
};

bool PresetUpdaterProcessStatus::on_attempt(int attempt, unsigned delay)
{
    if (m_dispatch_status_fn) {
        m_dispatch_status_fn(m_target, attempt, delay);
    }
    return get_canceled();
}
} // namespace Slic3r::Biz::PresetUpdater
