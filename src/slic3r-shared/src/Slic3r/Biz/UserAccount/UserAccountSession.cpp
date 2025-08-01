#include "Slic3r/Biz/UserAccount/UserAccountSession.hpp"

#include "Slic3r/Biz/Network/ServiceConfig.hpp"
#include "Slic3r/Biz/Network/Jwt.hpp"
#include "Slic3r/Log.hpp"

#include "fmt/format.h"
#include <nlohmann/json.hpp>

#include "libslic3r/format.hpp"

namespace Slic3r::Biz::UserAccount {

UserAccountSession::UserAccountSession(Platform::IMainThreadDispatcher& dispatcher) :
    UserAccountSessionDispatchBase{dispatcher}
{}

void UserAccountSession::set_tokens(
    const std::string& access_token,
    const std::string& refresh_token,
    const std::string& shared_session_key,
    long long expires_in
)
{
    // TODO: Add checks for empty tokens

    if (!access_token.empty()) {
        SPDLOG_INFO(
            "{} access_token: {} ... {}",
            __FUNCTION__,
            access_token.substr(0, 5),
            access_token.substr(access_token.size() - 5)
        );
    } else {
        SPDLOG_INFO("{} access_token empty!", __FUNCTION__);
    }

    {
        std::lock_guard<std::mutex> lock(m_credentials_mutex);
        m_access_token       = access_token;
        m_refresh_token      = refresh_token;
        m_shared_session_key = shared_session_key;
        m_next_token_timeout = /*std::time(nullptr) +*/ expires_in;
    }
    long long exp = expires_in - std::time(nullptr);
    dispatch_new_refresh_time(exp);
}

void UserAccountSession::do_clear(bool notify_owner)
{
    {
        std::lock_guard<std::mutex> lock(m_credentials_mutex);
        m_access_token.clear();
        m_refresh_token.clear();
        m_shared_session_key.clear();
    }
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        m_processing_enabled = false;
    }
    if (notify_owner) {
        dispatch_logged_out();
    }
}

void UserAccountSession::process_action_queue()
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        if (!m_processing_enabled)
            return;
        // SPDLOG_INFO("action queue: {} {}", m_priority_action_queue.size(), m_action_queue.size());
        if (m_priority_action_queue.empty() && m_action_queue.empty()) {
            // Update printers periodically.
            // ConnectStatus needs ConnectPrinterModels to be called once to has effect.
            enqueue_action_inner(UserAccountActionID::ConnectStatus, nullptr, nullptr, {});
        }
    }
    process_action_queue_inner();
}

void UserAccountSession::process_action_queue_inner()
{
    bool call_priority = false;
    bool call_standard = false;
    ActionQueueData selected_data;
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);

        // priority queue works even when tokens are empty or broken
        if (!m_priority_action_queue.empty()) {
            // Do a copy here even its costly. We need to get data outside m_session_mutex protected code to perform background operation over it.
            selected_data = m_priority_action_queue.front();
            m_priority_action_queue.pop_front();
            call_priority = true;
        } else if (this->is_initialized() && !m_action_queue.empty()) {
            // regular queue has to wait until priority fills tokens
            // Do a copy here even its costly. We need to get data outside m_session_mutex protected code to perform background operation over it.
            selected_data = m_action_queue.front();
            m_action_queue.pop();
            call_standard = true;
        }
    }
    if (call_priority || call_standard) {
        bool use_token = m_actions[selected_data.action_id]->get_requires_auth_token();
        m_actions[selected_data.action_id]->perform(
            this,
            use_token ? get_access_token() : std::string(),
            selected_data.success_callback,
            selected_data.fail_callback,
            selected_data.input,
            m_global_cancel
        );
        process_action_queue_inner();
    }
}

void UserAccountSession::on_log_in_code_response(const std::string& code, const std::string& code_verifier)
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        // Data we have
        const std::string REDIRECT_URI = "prusaslicer://login";
        std::string post_fields        = "code="
            + code
            + "&client_id="
            + Network::ServiceConfig::instance().account_client_id()
            + "&grant_type=authorization_code"
            + "&redirect_uri="
            + REDIRECT_URI
            + "&code_verifier="
            + code_verifier;

        m_processing_enabled = true;
        // fail fn might be cancel_queue here
        m_priority_action_queue.push_back(
            {UserAccountActionID::CodeForToken,
             std::bind(&UserAccountSession::token_success_callback, this, std::placeholders::_1),
             std::bind(&UserAccountSession::code_exchange_fail_callback, this, std::placeholders::_1),
             post_fields}
        );
    }
}

void UserAccountSession::enqueue_action(
    UserAccountActionID id,
    ActionSuccessFn success_callback,
    ActionFailFn fail_callback,
    const std::string& input
)
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        enqueue_action_inner(id, success_callback, fail_callback, input);
    }
}

void UserAccountSession::enqueue_test_with_refresh()
{
    // SPDLOG_INFO(__FUNCTION__);
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        // on test fail - try refresh
        m_processing_enabled = true;
        m_priority_action_queue.push_back(
            {UserAccountActionID::TestAccessToken,
             nullptr,
             std::bind(&UserAccountSession::enqueue_refresh, this, std::placeholders::_1),
             {}}
        );
    }
}

void UserAccountSession::enqueue_refresh(const std::string& body)
{
    // SPDLOG_INFO(__FUNCTION__);
    // TODO before or after push_back?
    dispatch_enqueued_refresh();
    std::string post_fields;
    {
        std::lock_guard<std::mutex> lock(m_credentials_mutex);
        assert(!m_refresh_token.empty());
        post_fields =
            "grant_type=refresh_token"
            "&client_id="
            + Network::ServiceConfig::instance().account_client_id()
            + "&refresh_token="
            + m_refresh_token;
    }
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        m_priority_action_queue.push_back(
            {UserAccountActionID::RefreshToken,
             std::bind(&UserAccountSession::token_success_callback, this, std::placeholders::_1),
             std::bind(&UserAccountSession::refresh_fail_callback, this, std::placeholders::_1),
             post_fields}
        );
    }
}

void UserAccountSession::enqueue_refresh_race(const std::string& refresh_token_from_store)
{
    // SPDLOG_INFO(__FUNCTION__);
    // TODO before or after push_back?
    dispatch_enqueued_refresh();
    std::string post_fields;
    {
        std::lock_guard<std::mutex> lock(m_credentials_mutex);
        assert(!m_refresh_token.empty());
        post_fields =
            "grant_type=refresh_token"
            "&client_id="
            + Network::ServiceConfig::instance().account_client_id()
            + "&refresh_token="
            + (refresh_token_from_store.empty() ? m_refresh_token : refresh_token_from_store);
    }
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        m_priority_action_queue.push_back(
            {UserAccountActionID::RefreshToken,
             std::bind(&UserAccountSession::token_success_callback, this, std::placeholders::_1),
             std::bind(&UserAccountSession::refresh_fail_soft_callback, this, std::placeholders::_1),
             post_fields}
        );
    }
}

bool UserAccountSession::is_enqueued(UserAccountActionID action_id) const
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        return std::any_of(
            std::begin(m_priority_action_queue),
            std::end(m_priority_action_queue),
            [action_id](const ActionQueueData& item) { return item.action_id == action_id; }
        );
    }
}

void UserAccountSession::enqueue_action_inner(
    UserAccountActionID id,
    ActionSuccessFn success_callback,
    ActionFailFn fail_callback,
    const std::string& input
)
{
    m_processing_enabled = true;
    m_action_queue.push({id, success_callback, fail_callback, input});
}

void UserAccountSession::refresh_fail_callback(const std::string& body)
{
    do_clear(true);
    cancel_queue();
    dispatch_action_fail(ActionFailType::Reset, body);
}

void UserAccountSession::refresh_fail_soft_callback(const std::string& body)
{
    // We do not clear tokens here, only notify token management
    cancel_queue();
    // Note: body cannot be moved to here from Action, it is used in the next call.
    dispatch_race_lost(body);
}

void UserAccountSession::cancel_queue()
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        m_priority_action_queue.clear();
        while (!m_action_queue.empty()) {
            m_action_queue.pop();
        }
    }
}

void UserAccountSession::code_exchange_fail_callback(const std::string& body)
{
    SPDLOG_INFO("Access token refresh failed, body: {}", body);
    do_clear(true);
    cancel_queue();
    dispatch_action_fail(ActionFailType::Reset, body);
}

void UserAccountSession::token_success_callback(const std::string& body)
{
    // No need to use lock m_session_mutex here

    // This is here to prevent performing refresh again until UUserAccountActionID::UserIdAfterTokenSuccess is performed.
    // If refresh with stored token was enqueued during performing one we are in its success_callback,
    // It would fail and prevent UserAccountActionID::UserID to write this tokens to store.
    remove_from_queue(UserAccountActionID::RefreshToken);

    SPDLOG_INFO("{} Access token refreshed", __FUNCTION__);
    // Data we need
    std::string access_token, refresh_token, shared_session_key;
    try {
        nlohmann::json j = nlohmann::json::parse(body);

        if (j.contains("access_token"))
            access_token = j["access_token"].get<std::string>();
        if (j.contains("refresh_token"))
            refresh_token = j["refresh_token"].get<std::string>();
        if (j.contains("shared_session_key"))
            shared_session_key = j["shared_session_key"].get<std::string>();
    } catch (const std::exception&) {
        std::string msg = "Could not parse server response after code exchange.";
        dispatch_action_fail(ActionFailType::Reset, body);
        return;
    }

    int expires_in = Network::Jwt::get_exp_seconds(access_token);
    if (access_token.empty() || refresh_token.empty() || shared_session_key.empty() || expires_in <= 0)
    {
        // just debug msg, no need to translate
        std::string msg = fmt::format(
            "Failed read tokens after POST.\nAccess token: {}\nRefresh token: {}\nShared session token: {}\nbody: {}",
            access_token,
            refresh_token,
            shared_session_key,
            body
        );
        {
            std::lock_guard<std::mutex> lock(m_credentials_mutex);
            m_access_token       = std::string();
            m_refresh_token      = std::string();
            m_shared_session_key = std::string();
            m_next_token_timeout = 0;
        }
        dispatch_action_fail(ActionFailType::Reset, body);
        return;
    }

    if (!access_token.empty()) {
        SPDLOG_INFO(
            "{} access_token: {} ... {}",
            __FUNCTION__,
            access_token.substr(0, 5),
            access_token.substr(access_token.size() - 5)
        );
    } else {
        SPDLOG_INFO("{} access_token empty!", __FUNCTION__);
    }

    {
        std::lock_guard<std::mutex> lock(m_credentials_mutex);
        m_access_token       = access_token;
        m_refresh_token      = refresh_token;
        m_shared_session_key = shared_session_key;
        m_next_token_timeout = std::time(nullptr) + expires_in;
    }
    enqueue_action(UserAccountActionID::UserIdAfterTokenSuccess, nullptr, nullptr, {});
    dispatch_new_refresh_time(expires_in);
}

void UserAccountSession::remove_from_queue(UserAccountActionID action_id)
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);

        auto it = std::find_if(
            std::begin(m_priority_action_queue),
            std::end(m_priority_action_queue),
            [action_id](const ActionQueueData& item) { return item.action_id == action_id; }
        );
        while (it != m_priority_action_queue.end()) {
            m_priority_action_queue.erase(it);
            it = std::find_if(
                std::begin(m_priority_action_queue),
                std::end(m_priority_action_queue),
                [action_id](const ActionQueueData& item) { return item.action_id == action_id; }
            );
        }
    }
}

} // namespace Slic3r::Biz::UserAccount
