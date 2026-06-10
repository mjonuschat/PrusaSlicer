#include "Slic3r/App/Browser/BrowserLogicPhysicalPrinter.hpp"

#include "Slic3r/Assert.hpp"
#include <Slic3r/Log.hpp>

#include <nlohmann/json.hpp>
#include <fmt/format.h>

namespace Slic3r::App::Browser {

BrowserLogicPhysicalPrinter::BrowserLogicPhysicalPrinter(
    const std::string& url, 
    const std::string& api_key, 
    const std::string& usr, 
    const std::string& psk) 
    : AbstractBrowserLogic(url, {"ExternalApp"}, "other_loading", "other_error")
    , m_api_key(api_key)
    , m_usr(usr)
    , m_psk(psk)
{
    m_events["reloadHomePage"] = std::bind(&BrowserLogicPhysicalPrinter::on_reload_event, this, std::placeholders::_1);
    m_events["appQuit"] = std::bind(&BrowserLogicPhysicalPrinter::on_dummy_event, this, std::placeholders::_1);
    m_events["appMinimize"] = std::bind(&BrowserLogicPhysicalPrinter::on_dummy_event, this, std::placeholders::_1);
}

std::vector<BrowserLogicCommand> 
BrowserLogicPhysicalPrinter::on_navigation_request_webview_event(const std::string& new_url, const std::string& current_url)
{
    SPDLOG_DEBUG("{} {}", __FUNCTION__, new_url);
    std::vector<BrowserLogicCommand> result;

    if (new_url.find(m_url) == 0) {
        m_reached_default_url = true;
        if (new_url == current_url) {
            m_styles_defined = false;
        }

        if (!m_api_key_sent && !m_usr.empty() && !m_psk.empty()) {
            auto cred_cmds = send_credentials();
            result.insert(result.end(), cred_cmds.begin(), cred_cmds.end());
        }
    } else if (m_reached_default_url && new_url.find("/web/" + m_loading_html) != std::string::npos) {
        // Veto back button to loading screen
        result.emplace_back(BrowserLogicCommandType::Veto, std::string());
        return result;
    }

    return result;
}

std::vector<BrowserLogicCommand> BrowserLogicPhysicalPrinter::on_loaded_webview_event(const std::string& url)
{
    SPDLOG_DEBUG("{} {}", __FUNCTION__, url);
    std::vector<BrowserLogicCommand> result;

    if (url.find("/web/" + m_loading_html) != std::string::npos && m_load_default_url) {
        m_load_default_url = false;
        result.emplace_back(BrowserLogicCommandType::LoadURL, m_url);
        return result;
    }

    if (url.find(m_url) == 0) {
        emplace_define_css_commands(result);
    } else {
        m_styles_defined = false;
    }

    result.emplace_back(BrowserLogicCommandType::SetLoadDefaultURLOnErrorFalse, std::string());

    if (!m_api_key.empty() && !m_api_key_sent) {
        auto key_cmds = send_api_key();
        result.insert(result.end(), key_cmds.begin(), key_cmds.end());
    }

    return result;
}

std::vector<BrowserLogicCommand> BrowserLogicPhysicalPrinter::on_script_message_webview_event(const std::string& message)
{
    SPDLOG_DEBUG("{}", __FUNCTION__);
    std::string event_string;
    try {
        nlohmann::json j = nlohmann::json::parse(message);
        
        if (!j.contains("event")) {
            SPDLOG_ERROR("Received invalid message from Physical Printer (missing event). Message: {}", message);
            return {};
        }
        
        if (!j["event"].is_string()) {
            SPDLOG_ERROR("Received invalid message from Physical Printer ('event' field is not a string). Message: {}", message);
            return {};
        }
        
        event_string = j["event"].get<std::string>();
    } catch (const nlohmann::json::exception& e) {
        SPDLOG_ERROR("Could not parse Physical Printer message. {}", e.what());
        return {};
    }

    if (m_events.find(event_string) == m_events.end()) {
        SPDLOG_WARN("Physical Printer Request has no handling function. Event: {}", event_string);
        return {};
    }

    return m_events[event_string](message);
}

std::vector<BrowserLogicCommand> BrowserLogicPhysicalPrinter::send_api_key()
{
    SPDLOG_DEBUG("{}", __FUNCTION__);
    m_api_key_sent = true;
    std::vector<BrowserLogicCommand> result;

    // Serialize via nlohmann::json to safely escape quotes/control characters and wrap in double quotes
    std::string safe_api_key = nlohmann::json(m_api_key).dump();

    std::string script = fmt::format(R"(
        if (window.originalFetch === undefined) {{
            console.log('Patching fetch with API key');
            window.originalFetch = window.fetch;
            window.fetch = function(input, init = {{}}) {{
                init.headers = init.headers || {{}};
                init.headers['X-Api-Key'] = sessionStorage.getItem('apiKey');
                console.log('Patched fetch', input, init);
                return window.originalFetch(input, init);
            }};
        }}
        sessionStorage.setItem('authType', 'ApiKey');
        sessionStorage.setItem('apiKey', {});
    )", safe_api_key);

    result.emplace_back(BrowserLogicCommandType::RunScript, script);
    result.emplace_back(BrowserLogicCommandType::DoReload, std::string());
    result.emplace_back(BrowserLogicCommandType::ClearBasicAuth, std::string());
    
    return result;
}

std::vector<BrowserLogicCommand> BrowserLogicPhysicalPrinter::send_credentials()
{
    SPDLOG_DEBUG("{}", __FUNCTION__);
    m_api_key_sent = true;
    std::vector<BrowserLogicCommand> result;

    result.emplace_back(BrowserLogicCommandType::RunScript, 
        "sessionStorage.removeItem('authType'); sessionStorage.removeItem('apiKey'); console.log('Session Storage cleared');");

    nlohmann::json payload = {
        {"username", m_usr},
        {"password", m_psk}
    };
    result.emplace_back(BrowserLogicCommandType::SetBasicAuth, payload.dump());
        result.emplace_back(BrowserLogicCommandType::DoReload, std::string());

    return result;
}

void BrowserLogicPhysicalPrinter::emplace_define_css_commands(std::vector<BrowserLogicCommand>& res)
{
    if (m_styles_defined) return;
    SPDLOG_DEBUG("{}", __FUNCTION__);
    m_styles_defined = true;
    
    std::string script;

#if defined(__APPLE__)
    script += R"(
        document.addEventListener('keydown', function (event) {
            if (event.key === 'F5' || (event.ctrlKey && event.key === 'r') || (event.metaKey && event.key === 'r')) {
                window.ExternalApp.postMessage(JSON.stringify({ event: 'reloadHomePage', fromKeyboard: true}));
            }
            if (event.metaKey && event.key === 'q') {
                window.ExternalApp.postMessage(JSON.stringify({ event: 'appQuit'}));
            }
            if (event.metaKey && event.key === 'm') {
                window.ExternalApp.postMessage(JSON.stringify({ event: 'appMinimize'}));
            }
        });
    )";
#endif

    if (!script.empty()) {
        res.emplace_back(BrowserLogicCommandType::RunScript, script);
    }
}

std::vector<BrowserLogicCommand> BrowserLogicPhysicalPrinter::on_reload_event(const std::string& message_data)
{
    SPDLOG_DEBUG("{}", __FUNCTION__);
    try {
        nlohmann::json j = nlohmann::json::parse(message_data);
        if (j.contains("fromKeyboard") && j["fromKeyboard"].is_boolean() && j["fromKeyboard"].get<bool>()) {
            return {{BrowserLogicCommandType::DoReload, {}}};
        } else {
            return {{BrowserLogicCommandType::LoadURL, m_url}};
        }
    } catch (const nlohmann::json::exception& e) {
        SPDLOG_ERROR("Could not parse Physical Printer reload message. {}", e.what());
        return {};
    }
}

} // namespace Slic3r::App::Browser