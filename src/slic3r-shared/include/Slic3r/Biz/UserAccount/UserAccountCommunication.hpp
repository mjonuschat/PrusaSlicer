#pragma once

#include "Slic3r/Biz/UserAccount/UserAccountCommunicationTokenBase.hpp"

namespace Slic3r::Biz::UserAccount {
/**
 * Owner of UserAccountSession.
 * Enqueues session actions.
 */
class UserAccountCommunication final : public UserAccountCommunicationTokenBase
{
public:
	UserAccountCommunication(Platform::IMainThreadDispatcher& dispatcher);
	~UserAccountCommunication() = default;

	UserAccountCommunication(const UserAccountCommunication& ) = delete;
    UserAccountCommunication(UserAccountCommunication&& other) = delete;
    UserAccountCommunication& operator=(const UserAccountCommunication& ) = delete;
    UserAccountCommunication& operator=(UserAccountCommunication&& other) = delete;

    /**
     * @brief Logs out of User Account, tokens are thrown out, all other running apps gets message to log out.
     */
    void do_log_out(bool notify_owner);
    
    /**
     * @brief Returns url to be displayed in browser for logging in.
     * @param lang_code Language code for localization. 2 letters.
     * @param generate_code_verifier If true, code verifier is generated and stored for later use. If false, older verifier is used. Should be false if service is not Prusa Account.
     * @param service Service that user selected to log in with. Returned url will be redirecting to the service. Default empty service is Prusa Account log in.
     * Use case: User selects "log in" - this function is called to get url to be displayed in dialog. 
     * There user selects "Google" - This function is called with "Google" as service and returns url to be displayed in external browser.
     */
    std::string on_log_in_request(const std::string& lang_code, bool generate_code_verifier, const std::string& service);
    
    /**
     * @brief Passes code from browser to finish logging in.
     */
    void on_log_in_code_response(const std::string& url_message);

    /**
     * @brief Returns true if logging in is finalized.
     */
    bool is_logged_in() const;

    std::string access_token() const;
    
    /**
     * @brief Returns path to avatar (even if it does not exists). 
     */
    boost::filesystem::path avatar() const;

    /**
     * @brief Uses data to download avatar.
     */
    void on_avatar_url(const std::string& data);

    /**
     * @brief Stores avatar into file.
     */
    void on_avatar_success(std::string&& data) const;

    void request_printables_secret_token();

private:
    std::string m_code_verifier;
    std::string m_avatar_extension;
};
}