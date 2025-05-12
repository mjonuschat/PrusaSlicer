#include "Slic3r/Biz/UserAccount/UserAccountAction.hpp"

#include "Slic3r/Biz/Network/IHttp.hpp"
#include "Slic3r/Log.hpp"

#include "libslic3r/format.hpp"

namespace Slic3r::Biz::UserAccount {

void UserAccountActionPost::perform(IUserAccountActionCallbacks* callbacks, /*UNUSED*/ const std::string& access_token, ActionSuccessFn success_callback, ActionFailFn fail_callback, const std::string& input, std::atomic_bool& global_cancel) const
{
    std::string url = m_url;
    SPDLOG_INFO("{}: {}", __FUNCTION__, url);
    
    // TODO: callbacks pointer does not look safe
    auto retry_fn = [&global_cancel, &callbacks](Network::IHttp::Retry retry, bool& cancel ) {
        SPDLOG_INFO("Retry attempt {}:{} ms to next attempt",  retry.attempt, retry.ms_to_next_attempt);
        if (retry.attempt > 1 && retry.just_tried) {
            callbacks->on_action_retry(std::move(retry));
        }
        cancel = global_cancel;
    };

    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Post, std::move(url), retry_fn);
    if (!input.empty())
        http->set_post_body(input);
    http->header("Content-type", "application/x-www-form-urlencoded")
    .on_error([fail_callback](std::string body, std::string error, unsigned status) {
        SPDLOG_INFO("UserActionPost::perform on_error");
        if (fail_callback)
            fail_callback(body);
    })
    .on_complete([success_callback](std::string body, unsigned status) {
        SPDLOG_INFO("UserActionPost::perform on_complete");
        if (success_callback)
            success_callback(body);
    })
    .perform_sync(Network::HttpRetryOpt::default_retry());
}

void UserAccountActionGetWithEvent::perform(IUserAccountActionCallbacks* callbacks, const std::string& access_token, ActionSuccessFn success_callback, ActionFailFn fail_callback, const std::string& input, std::atomic_bool& global_cancel) const
{
    std::string url = m_url;
    SPDLOG_INFO("{}: {}", __FUNCTION__, url);
    
    // TODO: callbacks pointer does not look safe
    auto retry_fn = [&global_cancel, &callbacks](Network::IHttp::Retry retry, bool& cancel ) {
        SPDLOG_INFO("Retry attempt {}:{} ms to next attempt",  retry.attempt, retry.ms_to_next_attempt);
        if (retry.attempt > 1 && retry.just_tried) {
            callbacks->on_action_retry(std::move(retry));
        }
        cancel = global_cancel;
    };

    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Get, std::move(url), retry_fn);
    if (!input.empty())
        http->set_post_body(input);
    if (!access_token.empty()) {
        http->header("Authorization", "Bearer " + access_token);
#ifndef _NDEBUG
        // In debug mode, also verify the token expiration
        // This is here to help with "dev" accounts with shorten (sort of faked) expiration time
        // The /api/v1/me will accept these tokens even if these are fake-marked as expired
        /*
        if (!Utils::verify_exp(access_token) && fail_callback) {
            fail_callback("Token Expired");
        }
        */
#endif
    }
    http->on_error([fail_callback, action_name = &m_action_name, &callbacks, fail_type = m_fail_type](std::string body, std::string error, unsigned status) {
        SPDLOG_INFO("UserActionGetWithEvent::perform on_error");
        if (fail_callback)
            fail_callback(body);
        std::string message = format("%1% action failed (%2%): %3%", action_name, std::to_string(status), body);
        if (fail_type != ActionFailType::None) {
            callbacks->on_action_fail(fail_type, std::move(message));
        }
    })
    .on_complete([success_callback, &callbacks, success_type = m_success_type](std::string body, unsigned status) {
        SPDLOG_INFO("UserActionGetWithEvent::perform on_complete");
        if (success_callback)
            success_callback(body);
        if (success_type != ActionSuccessType::None) {
            callbacks->on_action_success(success_type, std::move(body));
        }
    })
    .perform_sync(Network::HttpRetryOpt::default_retry());
}
	
} // namespace Slic3r::Biz::UserAccount 