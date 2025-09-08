#pragma once

#include "Slic3r/Biz/UserAccount/IUserAccountListener.hpp"

#include <wx/panel.h>

namespace Slic3r::App::WX::WebView {

class AbstractWebViewPanel : public wxPanel, public Biz::UserAccount::IUserAccountListener
{
public:
    AbstractWebViewPanel(wxWindow* parent) :
        wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
    {}

    virtual ~AbstractWebViewPanel() = default;

    // IUserAccountListener;
    void on_user_account_id_success(bool is_refresh, const std::string& username) override {}

    void on_user_account_logged_out() override {}

    void on_user_account_will_refresh() override {}

    void on_user_account_action_retry(
        const Biz::Network::IHttp::Retry& retry,
        std::function<void(void)> cancel_callback
    ) override
    {}
};

} // namespace Slic3r::App::WX::WebView
