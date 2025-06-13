#include "Slic3r/Biz/UserAccount/UserAccountCommunication.hpp"
#include "Slic3r/Biz/UserAccount/UserAccountCodeChallengeGenerator.hpp"
#include "Slic3r/Biz/Network/ServiceConfig.hpp"
#include "Slic3r/Biz/Network/IHttp.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/Biz/Directories.hpp"

#include "fmt/format.h"

#include <boost/filesystem/operations.hpp>
#include <boost/nowide/cstdio.hpp>

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
    
    std::string params = fmt::format("embed=1&client_id={}&response_type=code&code_challenge={}&code_challenge_method=S256&scope=basic_info&redirect_uri={}&language={}", CLIENT_ID, code_challenge, REDIRECT_URI, language);
    if (service.empty()){
        result_url = fmt::format("{}/o/authorize/?{}&choose_account=1", AUTH_HOST, params);
    } else {
        result_url = fmt::format("{}/login/{}?next=/o/authorize/?{}", AUTH_HOST, service, params);
    }
    
    return result_url;
}

void UserAccountCommunication::on_log_in_code_response(const std::string& url_message)
{
    const std::string code = get_code_from_message(url_message);
    m_session.on_log_in_code_response(code, m_code_verifier);
    wakeup_session_thread();
}

bool UserAccountCommunication::is_logged_in() const
{
    return !username().empty();
}

std::string UserAccountCommunication::access_token() const
{
    return m_session.get_access_token();
}

boost::filesystem::path UserAccountCommunication::avatar() const
{
    if (is_logged_in()) {
        const std::string filename = "slic3r3-avatar-" + std::to_string(Platform::PlatformServices::instance().app_hash()) + m_avatar_extension;
        return boost::filesystem::temp_directory_path() / filename;
    } else {
        return  boost::filesystem::path(Utils::resources_dir()) / "icons" / "user.svg";
    }
}
void UserAccountCommunication::on_avatar_url(const std::string& data)
{ 
    const boost::filesystem::path server_file(data);
    m_avatar_extension = server_file.extension().string();
    ASSERT(m_session.is_initialized());
    m_session.enqueue_action(UserAccountActionID::Avatar, nullptr, nullptr, data);
    wakeup_session_thread();
}

void UserAccountCommunication::on_avatar_success(std::string&& data) const
{
    ASSERT(is_logged_in());
    boost::filesystem::path path = avatar();
    FILE* file; 
    file = boost::nowide::fopen(path.generic_string().c_str(), "wb");
    if (file == NULL) {
        SPDLOG_ERROR("Failed to create file to store avatar picture at: {}", path.string());
        return;
    }
    fwrite(data.c_str(), 1, data.size(), file);
    fclose(file);
}


} // namespace Slic3r::Biz::UserAccount 