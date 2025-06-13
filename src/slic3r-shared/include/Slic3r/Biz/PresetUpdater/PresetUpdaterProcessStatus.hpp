#pragma once

#include "Slic3r/Biz/Network/IHttp.hpp"

#include <memory>
#include <string>
#include <map>
#include <functional>

#include <jthread/JThread.hpp>

namespace Slic3r::Biz::PresetUpdater {

class PresetUpdaterProcessStatus
{
public:
    typedef std::function<void(const std::string& /* target */, int /* attempt */, unsigned /* delay */)> StatusChangedFn;
    enum class PresetUpdaterRetryPolicy
    {
        PURP_5_TRIES,
        PURP_NO_RETRY,
    };
    // called from PresetUpdaterWrapper
    PresetUpdaterProcessStatus(JThread::StopToken& stop_token, StatusChangedFn status_fn) 
        : m_stop_token(stop_token)
        , m_dispatch_status_fn(status_fn)
    {}
    PresetUpdaterProcessStatus(PresetUpdaterProcessStatus&&) = delete;
    PresetUpdaterProcessStatus(const PresetUpdaterProcessStatus&) = delete;
    PresetUpdaterProcessStatus& operator = (PresetUpdaterProcessStatus&&) = delete;
    PresetUpdaterProcessStatus& operator = (const PresetUpdaterProcessStatus&) = delete;
    ~PresetUpdaterProcessStatus() = default;
    
    bool get_canceled() const { return m_stop_token.stop_requested();/*m_canceled.load();*/ }

    void set_error(const std::string& error_msg) {m_error = error_msg; }
    bool has_error() const { return !m_error.empty(); }
    std::string get_error() const { return m_error; }

    void set_warning(std::string msg) {m_warnings.emplace_back(msg);}
    bool has_warning() const { return !m_warnings.empty(); }
    std::vector<std::string>& get_warings() { return m_warnings; }
    std::string warnings_dump() const 
    { 
        std::string ret;
        for (const std::string& w : m_warnings)
            ret += w + "\n";
        return ret;
    }

    Network::HttpRetryOpt get_retry_policy() const { return m_retry_policy; }

    void set_access_token(const std::string& token) { m_access_token = token; } 
    std::string access_token() const { return m_access_token; }

    /// This set function will be called inside worker thread!
    /// So the function itself should implement calling event to UI.
    void set_target(const std::string& target) { m_target = target; }
    bool on_attempt(int attempt, unsigned delay);

private:
    // Any cancelation
    //std::atomic<bool> m_canceled {false};
    // Only in case of destructor of other imminent stop.
    //std::atomic<bool> m_force_canceled {false};
    std::string m_error;
    std::vector<std::string> m_warnings;
    Network::HttpRetryOpt m_retry_policy;
    static const std::map<PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy, Network::HttpRetryOpt> policy_map;
    std::string m_access_token;
    std::string m_target;
    StatusChangedFn m_dispatch_status_fn;

    JThread::StopToken& m_stop_token;
   
};

} // namespace Slic3r::Biz::PresetUpdater