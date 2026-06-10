#pragma once

#include "Slic3r/App/Browser/AbstractBrowserLogic.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Slic3r::App::Browser {

class BrowserLogicPhysicalPrinter final : public AbstractBrowserLogic
{
public:
    BrowserLogicPhysicalPrinter(const std::string& url, 
                                const std::string& api_key, 
                                const std::string& usr, 
                                const std::string& psk);
    
    // AbstractBrowserLogic
    std::vector<BrowserLogicCommand> on_navigation_request_webview_event(const std::string& new_url, const std::string& current_url) override;
    std::vector<BrowserLogicCommand> on_script_message_webview_event(const std::string& message) override;
    std::vector<BrowserLogicCommand> on_loaded_webview_event(const std::string& url) override;

private:
    std::map<std::string, std::function<std::vector<BrowserLogicCommand>(const std::string&)>> m_events;

    std::vector<BrowserLogicCommand> on_reload_event(const std::string& message_data);
    std::vector<BrowserLogicCommand> on_dummy_event(const std::string& message_data) { return {}; }

    std::vector<BrowserLogicCommand> send_api_key();
    std::vector<BrowserLogicCommand> send_credentials();
    void emplace_define_css_commands(std::vector<BrowserLogicCommand>& res);

    std::string m_api_key;
    std::string m_usr;
    std::string m_psk;

    bool m_reached_default_url{false};
    bool m_styles_defined{false};
    bool m_api_key_sent{false};
    bool m_load_default_url{true};
};

} // namespace Slic3r::App::Browser