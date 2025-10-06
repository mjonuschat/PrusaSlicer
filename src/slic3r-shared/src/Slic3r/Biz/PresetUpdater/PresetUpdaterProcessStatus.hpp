#pragma once

#include "Slic3r/Biz/PresetUpdater/PresetUpdaterWarning.hpp"

#include "Slic3r/Biz/Network/IHttp.hpp"
#include "Slic3r/Log.hpp"

#include <memory>
#include <string>
#include <map>
#include <functional>

#include <jthread/JThread.hpp>

namespace Slic3r::Biz::PresetUpdater {

class PresetUpdaterProcessStatus
{
public:
    typedef std::function<void(const std::string& /* target */, int /* attempt */, unsigned /* delay */)>
        StatusChangedFn;
    enum class PresetUpdaterRetryPolicy
    {
        PURP_5_TRIES,
        PURP_NO_RETRY,
    };

    // called from PresetUpdaterWrapper
    PresetUpdaterProcessStatus(JThread::StopToken& stop_token, StatusChangedFn status_fn, bool ignore_hash = false) :
        m_dispatch_status_fn(status_fn),
        m_stop_token(stop_token),
        m_ignore_file_hash(ignore_hash)
    {}

    PresetUpdaterProcessStatus(PresetUpdaterProcessStatus&&)                 = delete;
    PresetUpdaterProcessStatus(const PresetUpdaterProcessStatus&)            = delete;
    PresetUpdaterProcessStatus& operator=(PresetUpdaterProcessStatus&&)      = delete;
    PresetUpdaterProcessStatus& operator=(const PresetUpdaterProcessStatus&) = delete;
    ~PresetUpdaterProcessStatus()                                            = default;

    bool get_canceled() const
    {
        bool canceled = m_stop_token.stop_requested();
        if (canceled)
        {
            SPDLOG_ERROR("CANCELED");
        }
        return canceled;
    }

    void set_error(const std::string& error_msg)
    {
        m_error = error_msg;
    }

    bool has_error() const
    {
        return !m_error.empty();
    }

    std::string get_error() const
    {
        return m_error;
    }

    void set_warning(const std::string& msg)
    {
        m_warnings.emplace_back(msg, m_warning_target_repo, m_warning_target_vendor);
    }

    bool has_warning() const
    {
        return !m_warnings.empty();
    }

    std::vector<PresetUpdaterWarning>& warnings()
    {
        return m_warnings;
    }

    std::string warnings_dump() const
    {
        std::string ret;
        for (const PresetUpdaterWarning& w : m_warnings)
            ret += w.string() + "\n";
        return ret;
    }

    Network::HttpRetryOpt get_retry_policy() const
    {
        return m_retry_policy;
    }

    void set_access_token(const std::string& token)
    {
        m_access_token = token;
    }

    std::string access_token() const
    {
        return m_access_token;
    }

    bool on_attempt(int attempt, unsigned delay);

    void set_warning_target(const std::string& repo, const std::string& vendor = {})
    {
        m_warning_target_repo   = repo;
        m_warning_target_vendor = vendor;
    }

    void clear_warning_target()
    {
        m_warning_target_repo   = {};
        m_warning_target_vendor = {};
    }

    void set_target(const std::string& target)
    {
        m_target = target;
    }

    bool has_warning(const std::string& repo, const std::string& vendor) const
    {
        for (const auto& warning : m_warnings) {
            if (warning.repo == repo) {
                if (warning.vendor.empty() || warning.vendor == vendor) {
                    return true;
                }
            }
        }
        return false;
    }

    bool ignore_file_hash () { return m_ignore_file_hash; }

private:
    std::string m_error;
    std::vector<PresetUpdaterWarning> m_warnings;
    std::string m_warning_target_repo;
    std::string m_warning_target_vendor;
    std::string m_target;
    Network::HttpRetryOpt m_retry_policy;
    static const std::map<PresetUpdaterProcessStatus::PresetUpdaterRetryPolicy, Network::HttpRetryOpt>
        policy_map;
    std::string m_access_token;
    StatusChangedFn m_dispatch_status_fn;

    JThread::StopToken& m_stop_token;

    bool m_ignore_file_hash;
};

} // namespace Slic3r::Biz::PresetUpdater
