#include "Slic3r/Biz/UserAccount/UserAccountSessionDispatchBase.hpp"


namespace Slic3r::Biz::UserAccount {
UserAccountSessionDispatchBase::UserAccountSessionDispatchBase(Platform::IMainThreadDispatcher& dispatcher)
	: m_dispatcher{dispatcher}
{
}

UserAccountSessionDispatchBase::~UserAccountSessionDispatchBase()
{
	ASSERT(
        m_dispatcher.is_closed(),
        "There must be no queued events (not even in the future),"
        " because they may remember the address of this instance!"
    );
}

void UserAccountSessionDispatchBase::on_action_retry(const Network::IHttp::Retry& retry)
{
    dispatch_action_retry(retry);
}

void UserAccountSessionDispatchBase::on_action_success(ActionSuccessType success_type, std::string body)
{
    dispatch_action_success(success_type, std::move(body));
}

void UserAccountSessionDispatchBase::on_action_fail(ActionFailType fail_type, std::string body)
{
    dispatch_action_fail(fail_type, std::move(body));
}

void UserAccountSessionDispatchBase::dispatch_action_retry(const Network::IHttp::Retry& retry)
{
    bool dispatched = m_dispatcher.dispatch_on_main_thread([this, retry]() {
        this->invoke_listeners<IUserAccountSessionListener>([retry](auto* listener) {
            listener->on_action_retry(retry);
        });
    });
}

void UserAccountSessionDispatchBase::dispatch_action_success(ActionSuccessType success_type, std::string body)
{
    bool dispatched = m_dispatcher.dispatch_on_main_thread([this, success_type, body = std::move(body)]() mutable{
        this->invoke_listeners<IUserAccountSessionListener>([success_type, body = std::move(body)](auto* listener) mutable {
            listener->on_action_success(success_type, std::move(body));
        });
    });
}

void UserAccountSessionDispatchBase::dispatch_action_fail(ActionFailType fail_type, std::string body)
{
    bool dispatched = m_dispatcher.dispatch_on_main_thread([this, fail_type, body = std::move(body)]() mutable {
        this->invoke_listeners<IUserAccountSessionListener>([fail_type, body = std::move(body)](auto* listener) mutable {
            listener->on_action_fail(fail_type, std::move(body));
        });
    });
}

void UserAccountSessionDispatchBase::dispatch_enqueued_refresh()
{
    bool dispatched = m_dispatcher.dispatch_on_main_thread([this]() {
        this->invoke_listeners<IUserAccountSessionListener>([](auto* listener) {
            listener->on_enqueued_refresh();
        });
    });
}

void UserAccountSessionDispatchBase::dispatch_new_refresh_time(long long exp)
{
    bool dispatched = m_dispatcher.dispatch_on_main_thread([this, exp]() {
        this->invoke_listeners<IUserAccountSessionListener>([exp](auto* listener) {
            listener->on_new_refresh_time(exp);
        });
    });
}

void UserAccountSessionDispatchBase::dispatch_race_lost(const std::string& body)
{
    bool dispatched = m_dispatcher.dispatch_on_main_thread([this, body]() {
        this->invoke_listeners<IUserAccountSessionListener>([body](auto* listener) {
            listener->on_race_lost(body);
        });
    });
}

void UserAccountSessionDispatchBase::dispatch_logged_out()
{
    bool dispatched = m_dispatcher.dispatch_on_main_thread([this]() {
        this->invoke_listeners<IUserAccountSessionListener>([](auto* listener) {
            listener->on_logged_out();
        });
    });

}

} // namespace Slic3r::Biz::UserAccount 