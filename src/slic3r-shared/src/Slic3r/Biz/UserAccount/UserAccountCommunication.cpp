#include "Slic3r/Biz/UserAccount/UserAccountCommunication.hpp"
#include "Slic3r/Biz/UserAccount/UserAccountCodeChallengeGenerator.hpp"
#include "Slic3r/Biz/Network/ServiceConfig.hpp"
#include "Slic3r/Biz/Network/IHttp.hpp"
#include "libassert/assert.hpp"

#include "libslic3r/format.hpp"

namespace Slic3r::Biz::UserAccount {

namespace {

std::string get_code_from_message(const std::string& url_message)
{
    size_t pos = url_message.rfind("code=");
    std::string out;
    for (size_t i = pos + 5; i < url_message.size(); i++) {
        const char& c = url_message[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            out+= c;
        else
            break;  
    }
    return out;
}
} // namespace

UserAccountCommunication::UserAccountCommunication(Platform::IMainThreadDispatcher& dispatcher)
    : UserAccountCommunicationTokenBase{dispatcher}
{
}

void UserAccountCommunication::do_log_out(bool notify_owner)
{
    do_clear(notify_owner);
}

std::string UserAccountCommunication::on_log_in_request(const std::string& lang_code, bool generate_code_verifier, const std::string& service)
{
   
    std::string result_url;
    const std::string AUTH_HOST = Network::ServiceConfig::instance().account_url();
    const std::string CLIENT_ID = Network::ServiceConfig::instance().account_client_id();
    const std::string REDIRECT_URI = "prusaslicer://login";

    UserAccountCodeChallengeGenerator ccg; 
    if (generate_code_verifier) {
        m_code_verifier = ccg.generate_verifier();
    }
    std::string code_challenge = ccg.generate_challenge(m_code_verifier);

    std::string language = lang_code;
    ASSERT(!language.empty(), "Language code must not be empty.");
    if (language.size() > 2) {
        language = language.substr(0,2);
    }
    
    std::string params = format("embed=1&client_id=%1%&response_type=code&code_challenge=%2%&code_challenge_method=S256&scope=basic_info&redirect_uri=%3%&language=%4%", CLIENT_ID, code_challenge, REDIRECT_URI, language);
    params = Network::IHttp::escape_string(params);
    if (service.empty()){
        result_url = format("%1%/o/authorize/?%2%", AUTH_HOST, params);
    } else {
        result_url = format("%1%/login/%2%?next=/o/authorize/?%3%", AUTH_HOST, service, params);
    }
    
    return result_url;
}

void UserAccountCommunication::on_log_in_code_response(const std::string& url_message)
{
    const std::string code = get_code_from_message(url_message);
    //m_session->on_log_in_code_response(code, m_code_verifier);
    //wakeup_session_thread();
}

bool UserAccountCommunication::is_logged_in() const
{
    return false;
}

} // namespace Slic3r::Biz::UserAccount 