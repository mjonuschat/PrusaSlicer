#include "Slic3r/Biz/UserAccount/UserAccountInteractor.hpp"

#include "Slic3r/Log.hpp"

#include <nlohmann/json.hpp>

namespace Slic3r::Biz::UserAccount {
UserAccountInteractor::UserAccountInteractor(Platform::IMainThreadDispatcher& dispatcher)
	: m_dispatcher{dispatcher}
    , m_communication{dispatcher}
{
    m_communication.add_session_listener(this);
}

UserAccountInteractor::~UserAccountInteractor()
{
	ASSERT(
        m_dispatcher.is_closed(),
        "There must be no queued events (not even in the future),"
        " because they may remember the address of this instance!"
    );
}


void UserAccountInteractor::do_log_out(bool notify_owner)
{
    m_communication.do_log_out(notify_owner);
}

std::string UserAccountInteractor::on_log_in_request(const std::string& lang_code, bool generate_code_verifier, const std::string& service/* = std::string()*/)
{
    return m_communication.on_log_in_request(lang_code, generate_code_verifier, service);
}

void UserAccountInteractor::on_log_in_code_response(const std::string& url_message)
{
    m_communication.on_log_in_code_response(url_message);
}

bool UserAccountInteractor::is_logged_in() const
{
    return m_communication.is_logged_in();
}

void UserAccountInteractor::on_read_token_store_message()
{

}

void UserAccountInteractor::on_action_retry(Network::IHttp::Retry retry)
{
    SPDLOG_INFO("UserAccountInteractor: Retry attempt {}: {} ms to next attempt",  retry.attempt, retry.ms_to_next_attempt); 
}

void UserAccountInteractor::on_action_success(ActionSuccessType success_type, std::string body) 
{
    SPDLOG_INFO("UserAccountInteractor: Action success({}): {}", static_cast<int>(success_type), body); 
    switch (success_type)
    {
    case Slic3r::Biz::UserAccount::ActionSuccessType::None:
         // Empty callback
        return;
    case Slic3r::Biz::UserAccount::ActionSuccessType::UserID:
    case Slic3r::Biz::UserAccount::ActionSuccessType::UserIDAfterToken:
        on_user_id(body);
        break;
    case Slic3r::Biz::UserAccount::ActionSuccessType::ConnectStatus:
        break;
    case Slic3r::Biz::UserAccount::ActionSuccessType::ConnectPrinterModels:
        break;
    case Slic3r::Biz::UserAccount::ActionSuccessType::Avatar:
        break;
    case Slic3r::Biz::UserAccount::ActionSuccessType::PrinterData:
        break;
    default:
        ASSERT(false, "Unknown success type");
        break;
    }
}

void UserAccountInteractor::on_action_fail(ActionFailType fail_type, std::string body)  
{
    SPDLOG_INFO("UserAccountInteractor: Action fail({}): {}", static_cast<int>(fail_type), body); 
    switch (fail_type)
    {
    case Slic3r::Biz::UserAccount::ActionFailType::None: 
        // Empty callback
        return;
    case Slic3r::Biz::UserAccount::ActionFailType::Fail:
        break;
    case Slic3r::Biz::UserAccount::ActionFailType::Reset:
        do_log_out(true);
        break;
    case Slic3r::Biz::UserAccount::ActionFailType::PrinterData:
        break;
    default:
        ASSERT(false, "Unknown fail type");
        break;
    }
}

void UserAccountInteractor::on_enqueued_refresh() 
{
    // Here information about refresh being enqueued should be passed to other components, f.e. Printables WebView.
}

void UserAccountInteractor::on_new_refresh_time(long long exp) 
{
    m_communication.set_refresh_time(exp);
}

void UserAccountInteractor::on_race_lost(const std::string& msg) 
{
    m_communication.on_race_lost(msg);
}

void UserAccountInteractor::on_logged_out()
{
    invoke_listeners<IUserAccountListener>([](auto* listener){
        listener->on_user_account_logged_out();
    });
}

void UserAccountInteractor::on_user_id(const std::string& body)
{
    SPDLOG_INFO("UserAccountInteractor: User ID message: {}", body);
    try {
        nlohmann::json j = nlohmann::json::parse(body);
        
        m_account_user_data.clear();
        for (const auto& [key, value] : j.items()) {
            if (value.is_string()) {
                m_account_user_data[key] = value.get<std::string>();
            }
        }
    } 
    catch (const std::exception&) {
       SPDLOG_INFO("UserIDUserAction Could not parse server response.");
        return;
    }

    if (m_account_user_data.find("public_username") == m_account_user_data.end()) {
        SPDLOG_ERROR("User ID message from PrusaAuth did not contain public_username. Login failed. Message data: {}", body);
        return;
    }
    std::string public_username = m_account_user_data["public_username"];
    m_communication.on_username_changed(public_username, true);

    // enqueue GET with avatar url
    /*
    if (m_account_user_data.find("avatar_small") != m_account_user_data.end()) {
        const boost::filesystem::path server_file(m_account_user_data["avatar_small"]);
        m_avatar_extension = server_file.extension().string();
        enqueue_avatar_new_action(m_account_user_data["avatar_small"]);
    } else {
        BOOST_LOG_TRIVIAL(error) << "User ID message from PrusaAuth did not contain avatar.";
    }
    // update printers list
    enqueue_connect_printer_models_action();
    */

    invoke_listeners<IUserAccountListener>([](auto* listener){
        listener->on_user_account_id_success();
    });
}

} // namespace Slic3r::Biz::UserAccount