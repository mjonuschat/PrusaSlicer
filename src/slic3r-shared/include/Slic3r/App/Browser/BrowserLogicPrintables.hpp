#pragma once

#include "Slic3r/App/Browser/AbstractBrowserLogic.hpp"

#include <functional>
#include <map>

namespace Slic3r::Biz {
class ProjectInteractor;
}
namespace Slic3r::App::Browser {

class BrowserLogicPrintables final : public AbstractBrowserLogic
{
public:
    BrowserLogicPrintables(Biz::ProjectInteractor& project_interactor);
    
    // AbstractBrowserLogic
    std::string access_token() override;
    std::vector<BrowserLogicCommand> on_navigation_request_webview_event(const std::string& new_url, const std::string& current_url) override;
    std::vector<BrowserLogicCommand> on_script_message_webview_event(const std::string& message) override;
    std::vector<BrowserLogicCommand> on_show_webview_event(bool show) override;
    std::vector<BrowserLogicCommand> on_loaded_webview_event(const std::string& url) override;
    std::vector<BrowserLogicCommand> on_load_default_url() override;
    std::vector<BrowserLogicCommand> on_user_account_id_success(bool is_refresh) override;
    std::vector<BrowserLogicCommand> on_user_account_logged_out() override;
    std::vector<BrowserLogicCommand> on_user_account_will_refresh() override;

private:
    Biz::ProjectInteractor& m_project_interactor;
    std::map<std::string, std::function<std::vector<BrowserLogicCommand>(const std::string&)>> m_events;

    std::vector<BrowserLogicCommand> on_printables_event_dummy(const std::string& message_data) { return {}; }
    std::vector<BrowserLogicCommand> on_printables_event_access_token_expired(const std::string& message_data);
    std::vector<BrowserLogicCommand> on_printables_event_print_gcode(const std::string& message_data);
    std::vector<BrowserLogicCommand> on_printables_event_download_file(const std::string& message_data);
    std::vector<BrowserLogicCommand> on_printables_event_slice_file(const std::string& message_data);
    std::vector<BrowserLogicCommand> on_printables_event_required_login(const std::string& message_data);
    std::vector<BrowserLogicCommand> on_printables_event_open_url(const std::string& message_data);
    std::vector<BrowserLogicCommand> on_webview_reload_event(const std::string& message_data);

    std::vector<BrowserLogicCommand> logout(const std::string& override_url = std::string());
    std::vector<BrowserLogicCommand> login(const std::string& access_token, const std::string& override_url = std::string());
    void emplace_define_css_commands(std::vector<BrowserLogicCommand>& res);
    void emplace_load_default_url_commands(std::vector<BrowserLogicCommand>& res);

    std::string script_hide_loading_overlay() const;
    std::string script_show_loading_overlay() const;
    std::string url_lang_theme(const std::string& url) const;

    //std::string url_lang_theme(const wxString& url) const;
    //void show_download_notification(const std::string& filename);

    bool m_reached_default_url {false};
    bool m_styles_defined {false};
    bool m_refreshing_token {false};
    bool m_load_default_url { true };
    bool m_remove_request_auth {false};
    std::string m_last_loaded_url;
};
} // namespace Slic3r::App::Browser 