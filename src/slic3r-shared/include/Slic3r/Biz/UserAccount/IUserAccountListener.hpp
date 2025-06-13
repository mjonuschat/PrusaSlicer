#pragma once

#include <string>

namespace Slic3r::Biz::UserAccount {

/**
 * @brief Interface for events going outside whole UserAccount logic.
 */
class IUserAccountListener {
public:
    virtual ~IUserAccountListener() = default;
    virtual void on_user_account_id_success(bool is_refresh) = 0;
    virtual void on_user_account_logged_out() = 0;
    virtual void on_user_account_will_refresh() = 0;
};
}
