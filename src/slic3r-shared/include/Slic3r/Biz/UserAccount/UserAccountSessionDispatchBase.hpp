#pragma once

#include "Slic3r/Biz/UserAccount/IUserAccountActionCallbacks.hpp"
#include "Slic3r/Biz/UserAccount/IUserAccountSessionListener.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"
#include "Slic3r/Biz/Platform/IMainThreadDispatcher.hpp"

namespace Slic3r::Biz::UserAccount {

/**
 * @brief Base class for dispatching user account session events.
 * Also accepts callbacks from the UserAccountAction and dispatches them to listeners.
 */
class UserAccountSessionDispatchBase : public IUserAccountActionCallbacks, public WithListeners<IUserAccountSessionListener>
{
public:
	UserAccountSessionDispatchBase(Platform::IMainThreadDispatcher& dispatcher);
	~UserAccountSessionDispatchBase();

    
    void on_action_retry(Network::IHttp::Retry retry) override;
    void on_action_success(ActionSuccessType success_type, std::string body) override;
    void on_action_fail(ActionFailType fail_type, std::string body) override;

	UserAccountSessionDispatchBase(const UserAccountSessionDispatchBase& ) = delete;
    UserAccountSessionDispatchBase(UserAccountSessionDispatchBase&& other) = delete;
    UserAccountSessionDispatchBase& operator=(const UserAccountSessionDispatchBase& ) = delete;
    UserAccountSessionDispatchBase& operator=(UserAccountSessionDispatchBase&& other) = delete;

protected:
    void dispatch_action_retry(Network::IHttp::Retry retry);
    void dispatch_action_success(ActionSuccessType success_type, std::string body);
    void dispatch_action_fail(ActionFailType fail_type, std::string body);
    void dispatch_enqueued_refresh();
    void dispatch_new_refresh_time(long long exp);
    void dispatch_race_lost(const std::string& body);
    void dispatch_logged_out();

private:
	Platform::IMainThreadDispatcher& m_dispatcher;
};
}