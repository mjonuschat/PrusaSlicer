#include "PresetUpdaterProcessStatus.hpp"

#include "../../Utils/Format.hpp"

using namespace std::chrono_literals;

namespace PresetManagement {

const std::map<PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy, Slic3r::HttpRetryOpt> PresetUpdaterProcessStatus::policy_map = {
    {PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_5_TRIES,     {500ms, 5s, 4}},
    {PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy::PURP_NO_RETRY,    {0ms}}
};

void PresetUpdaterProcessStatus::reset(PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy policy)
{
    if (auto it = policy_map.find(policy); it != policy_map.end()) {
        m_retry_policy = it->second;
    } else {
        assert(false);
        m_retry_policy = {0ms};
    }
    m_canceled.store(false);
    m_force_canceled.store(false);
    m_error.clear();
    m_warnings.clear();
    m_access_token.clear();
    m_target.clear();
    m_status_changed_fn;
}
bool PresetUpdaterProcessStatus::on_attempt(int attempt, unsigned delay)
{
    if (m_status_changed_fn) {
        m_status_changed_fn(m_target, attempt, delay);
    }
    return get_canceled();
}

} // PresetManagment