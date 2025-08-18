#include "Slic3r/App/Browser/BrowserLogicPrintables.hpp"

#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/Browser/BrowserLogicPrintablesToConnect.hpp"

#include "Slic3r/Biz/Network/ServiceConfig.hpp"
#include <Slic3r/Biz/Platform/PlatformServices.hpp>
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Network/IHttp.hpp"
#include "Slic3r/Assert.hpp"

#include <nlohmann/json.hpp>
#include <regex>

namespace Slic3r::App::Browser {

BrowserLogicPrintables::BrowserLogicPrintables(Biz::ProjectInteractor& project_interactor) :
    AbstractBrowserLogic(Biz::Network::ServiceConfig::instance().printables_url(), {"ExternalApp"}),
    m_project_interactor(project_interactor)
{
    m_events["accessTokenExpired"] = std::bind(&BrowserLogicPrintables::on_printables_event_access_token_expired, this, std::placeholders::_1);
    m_events["printGcode"] = std::bind(&BrowserLogicPrintables::on_printables_event_print_gcode, this, std::placeholders::_1);
    m_events["downloadFile"] = std::bind(&BrowserLogicPrintables::on_printables_event_download_file, this, std::placeholders::_1);
    m_events["sliceFile"] = std::bind(&BrowserLogicPrintables::on_printables_event_slice_file, this, std::placeholders::_1);
    m_events["requiredLogin"] = std::bind(&BrowserLogicPrintables::on_printables_event_required_login, this, std::placeholders::_1);
    m_events["openExternalUrl"] = std::bind(&BrowserLogicPrintables::on_printables_event_open_url, this, std::placeholders::_1);
    m_events["ready"] = std::bind(&BrowserLogicPrintables::on_printables_event_dummy, this, std::placeholders::_1);
    m_events["reloadHomePage"] = std::bind(&BrowserLogicPrintables::on_webview_reload_event, this, std::placeholders::_1);
}

std::string BrowserLogicPrintables::access_token()
{
    return m_project_interactor.user_account_interactor().access_token();
}

std::vector<BrowserLogicCommand>
BrowserLogicPrintables::on_navigation_request_webview_event(const std::string& new_url, const std::string& current_url)
{
    if (new_url.find(m_url) == 0) {
        m_reached_default_url = true;
        if (new_url == current_url) {
            // we need to do this to redefine css when reload is hit
            m_styles_defined = false;
        }
    } else if (m_reached_default_url && new_url.find("http") == 0) {
        SPDLOG_INFO("{} does not start with default url. Vetoing.", new_url);
        return {{BrowserLogicCommandType::Veto, {}}};
    } else if (m_reached_default_url && new_url.find("/web/" + m_loading_html) != std::string::npos)
    {
        // Do not allow back button to loading screen
        return {{BrowserLogicCommandType::Veto, {}}};
    }

    return {};
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_show_webview_event(bool show)
{
    std::vector<BrowserLogicCommand> result;
    result.emplace_back(BrowserLogicCommandType::SetLoadDefaultURLOnErrorTrue, std::string());

    // in case login changed, resend login / logout
    // DK: it seems to me, it is safer to do login / logout (where logout means requesting the page again)
    // on every show of panel,
    // than to keep information if we have printables page in same state as slicer in terms of login
    const std::string access_token = m_project_interactor.user_account_interactor().access_token();
    if (access_token.empty()) {
        result = logout(m_next_show_url);
    } else {
        result = login(access_token, m_next_show_url);
    }
    m_next_show_url.clear();

    return result;
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_loaded_webview_event(const std::string& url)
{
    m_last_loaded_url = url;
    std::vector<BrowserLogicCommand> result;
    if (url.find("/web/" + m_loading_html) != std::string::npos && m_load_default_url) {
        m_load_default_url = false;
        emplace_load_default_url_commands(result);
        return result;
    }

    if (url.find(m_url) == 0) {
        emplace_define_css_commands(result);
    } else {
        m_styles_defined = false;
    }

#ifdef _WIN32
    // This is needed only once after add_request_authorization
    if (m_remove_request_auth) {
        m_remove_request_auth = false;
        result.emplace_back(BrowserLogicCommandType::RemoveRequestAuthorization, std::string());
    }
#endif
    result.emplace_back(BrowserLogicCommandType::SetLoadDefaultURLOnErrorFalse, std::string());
    return result;
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_load_default_url()
{
    std::vector<BrowserLogicCommand> res;
    emplace_load_default_url_commands(res);
    return res;
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_user_account_id_success(bool is_refresh)
{
    if (m_load_default_url) {
        return {};
    }
    if (!is_refresh) {
        return login(m_project_interactor.user_account_interactor().access_token());
    }

    std::vector<BrowserLogicCommand> result;
    result.emplace_back(BrowserLogicCommandType::RunScript, script_hide_loading_overlay());

    std::string token = m_project_interactor.user_account_interactor().access_token();
    std::string script = "window.postMessage(JSON.stringify({event: 'accessTokenChange',token: '" + token + "'}));";
    result.emplace_back(BrowserLogicCommandType::RunScript, script);

    return result;
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_user_account_logged_out()
{
    if (m_load_default_url) {
        return {};
    }
    return logout();
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_user_account_will_refresh()
{
    if (m_load_default_url) {
        return {};
    }
    return {{BrowserLogicCommandType::RunScript, "window.postMessage(JSON.stringify({ event: 'accessTokenWillChange' }))"}};
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_script_message_webview_event(const std::string& message)
{
    std::string event_string;
    try {
        nlohmann::json j = nlohmann::json::parse(message);
        if (j.contains("event") && j["event"].is_string()) {
            event_string = j["event"].get<std::string>();
        }
    } catch (const nlohmann::json::parse_error& e) {
        SPDLOG_ERROR("Could not parse Printables message. {}", e.what());
        return {};
    }

    if (event_string.empty()) {
        SPDLOG_ERROR("Received invalid message from Printables (missing event). Message: {}", message);
        return {};
    }

    SPDLOG_INFO("Printables Request: {}", event_string);
    ASSERT(m_events.find(event_string) != m_events.end(), "There is an event that has no handling function.");
    return m_events[event_string](message);
}

std::vector<BrowserLogicCommand>
BrowserLogicPrintables::on_printables_event_access_token_expired(const std::string& message_data)
{
    // { "event": "accessTokenExpired")
    // There seems to be a situation where we get accessTokenExpired when there is active token from Slicer POW
    // We need get new token and freeze webview until its not refreshed
    if (m_refreshing_token) {
        return {};
    }
    m_refreshing_token = true;
    m_project_interactor.user_account_interactor().request_refresh();
    return {{BrowserLogicCommandType::RunScript, script_show_loading_overlay()}};
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_printables_event_print_gcode(const std::string& message_data)
{
    // { "event": "downloadFile", "url": "https://media.printables.com/somesecure.stl", "modelUrl": "https://www.printables.com/model/123" }
    std::string download_url;
    std::string model_url;
    try {
        nlohmann::json j = nlohmann::json::parse(message_data);
        if (j.contains("url") && j["url"].is_string()) {
            download_url = j["url"].get<std::string>();
        }
        if (j.contains("modelUrl") && j["modelUrl"].is_string()) {
            model_url = j["modelUrl"].get<std::string>();
        }
    } catch (const nlohmann::json::parse_error& e) {
        SPDLOG_ERROR("Could not parse Printables message. {}", e.what());
        return {};
    }
    DEBUG_ASSERT(!download_url.empty() && !model_url.empty(), "Faulty printables message.");

    std::string final_url = Biz::Network::ServiceConfig::instance().connect_printables_print_url() + "?url=" + Biz::Network::IHttp::escape_string(download_url);
    // TODO use final_url

    AppServices::instance().dialog_manager().show_webview_dialog(std::make_unique<Browser::BrowserLogicPrintablesToConnect>(final_url, m_project_interactor), &m_project_interactor);

    return {};
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_printables_event_download_file(const std::string& message_data)
{
    // { "event": "downloadFile", "url": "https://media.printables.com/somesecure.stl", "modelUrl": "https://www.printables.com/model/123" }
    std::string download_url;
    std::string model_url;
    try {
        nlohmann::json j = nlohmann::json::parse(message_data);
        if (j.contains("url") && j["url"].is_string()) {
            download_url = j["url"].get<std::string>();
        }
        if (j.contains("modelUrl") && j["modelUrl"].is_string()) {
            model_url = j["modelUrl"].get<std::string>();
        }
    } catch (const nlohmann::json::parse_error& e) {
        SPDLOG_ERROR("Could not parse Printables message. {}", e.what());
        return {};
    }
    DEBUG_ASSERT(!download_url.empty() && !model_url.empty(), "Faulty printables message.");

    std::string final_url = Biz::Network::ServiceConfig::instance().connect_printables_print_url() + "?url=" + Biz::Network::IHttp::escape_string(download_url);
    // TODO use final_url

    return {};
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_printables_event_slice_file(const std::string& message_data)
{
    // { "event": "downloadFile", "url": "https://media.printables.com/somesecure.stl", "modelUrl": "https://www.printables.com/model/123" }
    std::string download_url;
    std::string model_url;
    try {
        nlohmann::json j = nlohmann::json::parse(message_data);
        if (j.contains("url") && j["url"].is_string()) {
            download_url = j["url"].get<std::string>();
        }
        if (j.contains("modelUrl") && j["modelUrl"].is_string()) {
            model_url = j["modelUrl"].get<std::string>();
        }
    } catch (const nlohmann::json::parse_error& e) {
        SPDLOG_ERROR("Could not parse Printables message. {}", e.what());
        return {};
    }
    DEBUG_ASSERT(!download_url.empty() && !model_url.empty(), "Faulty printables message.");

    std::string final_url = Biz::Network::ServiceConfig::instance().connect_printables_print_url() + "?url=" + Biz::Network::IHttp::escape_string(download_url);
    // TODO use final_url

    return {};
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_printables_event_required_login(const std::string& message_data)
{
    return {};
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_printables_event_open_url(const std::string& message_data)
{
    std::string url;
    try {
        nlohmann::json j = nlohmann::json::parse(message_data);
        if (j.contains("url") && j["url"].is_string()) {
            url = j["url"].get<std::string>();
        }
    } catch (const nlohmann::json::parse_error& e) {
        SPDLOG_ERROR("Could not parse Printables message. {}", e.what());
        return {};
    }
    return {{BrowserLogicCommandType::OpenExternalBrowser, url}};
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::on_webview_reload_event(const std::string& message_data)
{
    // Event from our error page button or keyboard shortcut
    m_styles_defined = false;
    try {
        nlohmann::json j = nlohmann::json::parse(message_data);
        if (j.contains("fromKeyboard") && j["fromKeyboard"].is_boolean() && j["fromKeyboard"].get<bool>())
        {
            return {{BrowserLogicCommandType::DoReload, {}}};
        } else {
            std::vector<BrowserLogicCommand> res;
            emplace_load_default_url_commands(res);
            return res;
        }
    } catch (const nlohmann::json::parse_error& e) {
        SPDLOG_ERROR("Could not parse Printables message. {}", e.what());
        return {};
    }
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::logout(const std::string& override_url)
{
    std::vector<BrowserLogicCommand> result;
    m_refreshing_token = false;
    m_styles_defined   = false;
    result.emplace_back(BrowserLogicCommandType::RunScript, script_hide_loading_overlay());
    result.emplace_back(
        BrowserLogicCommandType::DeleteCookies,
        Biz::Network::ServiceConfig::instance().printables_url()
    );
    result.emplace_back(BrowserLogicCommandType::RunScript, "localStorage.clear();");

    std::string next_url = override_url.empty() ? url_lang_theme(m_last_loaded_url) : url_lang_theme(override_url);
#ifdef _WIN32
    result.emplace_back(BrowserLogicCommandType::LoadURL, next_url);
#else
    // We cannot do simple reload here, it would keep the access token in the header
    result.emplace_back(BrowserLogicCommandType::LoadRequest, next_url);
#endif //

    return result;
}

std::vector<BrowserLogicCommand> BrowserLogicPrintables::login(const std::string& access_token, const std::string& override_url)
{
    std::vector<BrowserLogicCommand> result;
    m_refreshing_token = false;
    m_styles_defined   = false;
    result.emplace_back(BrowserLogicCommandType::RunScript, script_hide_loading_overlay());
    // We cannot add token to header as when making the first request.
    // In fact, we shall not do request here, only run scripts.
    // postMessage accessTokenWillChange -> postMessage accessTokenChange -> window.location.reload();
    result.emplace_back(BrowserLogicCommandType::RunScript, "window.postMessage(JSON.stringify({ event: 'accessTokenWillChange' }))");
    result.emplace_back(BrowserLogicCommandType::RunScript, "window.postMessage(JSON.stringify({event: 'accessTokenChange',token: '" + access_token + "'}));");

    if (override_url.empty()) {
        result.emplace_back(BrowserLogicCommandType::RunScript, "window.location.reload();");
    } else {
        result.emplace_back(BrowserLogicCommandType::LoadURL, url_lang_theme(override_url));
    }
    return result;
}

std::string BrowserLogicPrintables::script_hide_loading_overlay() const
{
    return R"(
        function slic3r_hideLoadingOverlay() {
            const overlayDiv = document.getElementById('slic3r-loading-overlay');
            if (overlayDiv)
                overlayDiv.remove();
        }
        slic3r_hideLoadingOverlay();
    )";
}

std::string BrowserLogicPrintables::script_show_loading_overlay() const
{
    return R"(
        function slic3r_showLoadingOverlay() {
            const body = document.getElementsByTagName('body')[0];
            const overlayDiv = document.createElement('div');
            overlayDiv.className = 'slic3r-loading-overlay'
            overlayDiv.id = 'slic3r-loading-overlay';
            overlayDiv.innerHTML = '<div class="slic3r-loading-anim"></div>';
            body.appendChild(overlayDiv);
        }
        slic3r_showLoadingOverlay();
    )";
}

void BrowserLogicPrintables::emplace_define_css_commands(std::vector<BrowserLogicCommand>& res)
{
    if (m_styles_defined) {
        return;
    }
    m_styles_defined = true;

    std::string script = R"(
        // Loading overlay and Notification style
        var style = document.createElement('style');
        style.innerHTML = `
        body {}
        .slic3r-loading-overlay {
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: rgba(127 127 127 / 50%);
            z-index: 50;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .slic3r-loading-anim {
            width: 60px;
            aspect-ratio: 4;
            --_g: no-repeat radial-gradient(circle closest-side,#000 90%,#0000);
            background:
                    var(--_g) 0%   50%,
                    var(--_g) 50%  50%,
                    var(--_g) 100% 50%;
            background-size: calc(100%/3) 100%;
            animation: slic3r-loading-anim 1s infinite linear;
        }
        @keyframes slic3r-loading-anim {
            33%{background-size:calc(100%/3) 0%  ,calc(100%/3) 100%,calc(100%/3) 100%}
            50%{background-size:calc(100%/3) 100%,calc(100%/3) 0%  ,calc(100%/3) 100%}
            66%{background-size:calc(100%/3) 100%,calc(100%/3) 100%,calc(100%/3) 0%  }
        }
        .notification-popup {
            position: fixed;
            right: 10px;
            bottom: 10px;
            background-color: #333333; /* Dark background */
            padding: 10px;
            border-radius: 6px; /* Slightly rounded corners */
            color: #ffffff; /* White text */
            font-family: Arial, sans-serif;
            font-size: 12px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            box-shadow: 0px 4px 8px rgba(0, 0, 0, 0.3); /* Add a subtle shadow */
            min-width: 350px; 
            max-width: 350px;
            min-height: 50px;
        }
        .notification-popup div {
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
            padding-right: 20px; /* Add padding to make text truncate earlier */
        }
        .notification-popup b {
            color: #ffa500;
        }
        .notification-popup a:hover {
            text-decoration: underline; /* Underline on hover */
        }
        .notification-popup .close-button {
            display: inline-block;
            width: 20px;
            height: 20px;
            border: 2px solid #ffa500; /* Orange border for the button */
            border-radius: 4px;
            text-align: center;
            font-size: 16px;
            line-height: 16px;
            cursor: pointer;
            padding-top: 1px; 
        }
        .notification-popup .close-button:hover {
            background-color: #ffa500; /* Orange background on hover */
            color: #333333; /* Dark color for the "X" on hover */
        }
        .notification-popup .close-button:before {
            content: 'X';
            color: #ffa500; /* Orange "X" */
            font-weight: bold;
        }
        `;
        document.head.appendChild(style); 
    
        // Capture click on hypertext
        // Rewritten from mobileApp code
        (function() {
            const listenerKey = 'custom-click-listener';
            if (!document[listenerKey]) {
                document.addEventListener( 'click', function(event) {
                    const target = event.target.closest('a[href]');
                    if (!target) return; // Ignore clicks that are not on links
                    const url = target.href;
                    // Allow empty iframe navigation
                    if (url === 'about:blank') {
                        return; // Let it proceed
                    }
                    // Debug log for navigation
                    console.log(`Printables:onNavigationRequest: ${url}`);
                    // Handle all non-printables.com domains in an external browser
                    if (!/printables\.com/.test(url)) {
                        window.ExternalApp.postMessage(JSON.stringify({ event: 'openExternalUrl', url }))
                        event.preventDefault();
                    }
                    // Default: Allow navigation to proceed
                },true); // Capture the event during the capture phase
                document[listenerKey] = true;
            }
        })();
    )";
#if defined(__APPLE__)
    // WebView on Windows does read keyboard shortcuts
    // Thus doing f.e. Reload twice would make the oparation to fail
    script += R"(
        document.addEventListener('keydown', function (event) {
            if (event.key === 'F5' || (event.ctrlKey && event.key === 'r') || (event.metaKey && event.key === 'r')) {
                window.ExternalApp.postMessage(JSON.stringify({ event: 'reloadHomePage', fromKeyboard: 1}));
            }
            if (event.metaKey && event.key === 'q') {
                window.ExternalApp.postMessage(JSON.stringify({ event: 'appQuit'}));
            }
            if (event.metaKey && event.key === 'm') {
                window.ExternalApp.postMessage(JSON.stringify({ event: 'appMinimize'}));
            }
        });
    )";
#endif // defined(__APPLE__)
    res.emplace_back(BrowserLogicCommandType::RunScript, script);
}

std::string BrowserLogicPrintables::url_lang_theme(const std::string& url) const
{
    // situations and reaction:
    // 1) url is just a path (no query no fragment) -> query with lang and theme is added
    // 2) url has query that contains lang and theme -> query and lang values are modified
    // 3) url has query with just one of lang or theme -> query is modified and missing value is added
    // 4) url has query of query and fragment without lang and theme -> query with lang and theme is added to the end of query

    std::string url_string = url;
    std::string theme      = "dark"; // wxGetApp().dark_mode() ? "dark" : "light";
    std::string language   = "en"; // GUI::wxGetApp().current_language_code();
    if (language.size() > 2)
        language = language.substr(0, 2);

    // Replace lang and theme if already in url
    bool lang_found = false;
    std::regex lang_regex(R"((lang=)[^&#]*)");
    if (std::regex_search(url_string, lang_regex)) {
        url_string = std::regex_replace(url_string, lang_regex, "$1" + language);
        lang_found = true;
    }
    bool theme_found = false;
    std::regex theme_regex(R"((theme=)[^&#]*)");
    if (std::regex_search(url_string, theme_regex)) {
        url_string  = std::regex_replace(url_string, theme_regex, "$1" + theme);
        theme_found = true;
    }
    if (lang_found && theme_found)
        return url_string;

    // missing params string
    std::string new_params = lang_found ?
        "theme=" + theme :
        theme_found ?
        "lang=" + language :
        "lang=" + language + "&theme=" + theme;

    // Regex to capture query and optional fragment
    std::regex query_regex(R"((\?.*?)(#.*)?$)");

    if (std::regex_search(url_string, query_regex)) {
        // Append params before the fragment (if it exists)
        return std::regex_replace(url_string, query_regex, "$1&" + new_params + "$2");
    }
    std::regex fragment_regex(R"(#.*$)");
    if (std::regex_search(url_string, fragment_regex)) {
        // Add params before the fragment
        return std::regex_replace(url_string, fragment_regex, "?" + new_params + "$&");
    }

    return url_string + "?" + new_params;
}

void BrowserLogicPrintables::emplace_load_default_url_commands(std::vector<BrowserLogicCommand>& res)
{
    res.emplace_back(BrowserLogicCommandType::RunScript, script_hide_loading_overlay());
    m_styles_defined = false;
    std::string actual_default_url = url_lang_theme(Biz::Network::ServiceConfig::instance().printables_url() + "/homepage");
    const std::string access_token = m_project_interactor.user_account_interactor().access_token();
    // in case of opening printables logged out - delete cookies and localstorage to get rid of last login
    if (access_token.empty()) {
        res.emplace_back(
            BrowserLogicCommandType::DeleteCookies,
            Biz::Network::ServiceConfig::instance().printables_url()
        );
        res.emplace_back(BrowserLogicCommandType::AddUserScript, "localStorage.clear();");
        res.emplace_back(BrowserLogicCommandType::LoadURL, std::move(actual_default_url));
        return;
    }
    // add token to first request
#ifdef _WIN32
    res.emplace_back(BrowserLogicCommandType::AddRequestAuthorization, m_url);
    m_remove_request_auth = true;
    res.emplace_back(BrowserLogicCommandType::LoadURL, std::move(actual_default_url));
#else
    res.emplace_back(BrowserLogicCommandType::LoadRequest, std::move(actual_default_url));
#endif
}

} // namespace Slic3r::App::Browser
