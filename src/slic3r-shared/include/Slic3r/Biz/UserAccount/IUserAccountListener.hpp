#pragma once

#include "Slic3r/Biz/Network/IHttp.hpp"
#include <string>
#include <functional>

namespace Slic3r::Biz::UserAccount {

/**
 * @brief Interface for events going outside whole UserAccount logic.
 */
class IUserAccountListener
{
public:
    virtual ~IUserAccountListener() = default;
    virtual void on_user_account_id_success(bool is_refresh, const std::string& username) {};
    virtual void on_user_account_logged_out() {};
    virtual void on_user_account_will_refresh() {};
    virtual void on_user_account_action_retry(
        const Network::IHttp::Retry& retry,
        std::function<void(void)> cancel_callback
    ) {};
    virtual void on_printables_secret_token(const std::string& body) {};
};
} // namespace Slic3r::Biz::UserAccount
