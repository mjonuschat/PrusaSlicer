#include "WebViewPlatformUtils.hpp"

#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/Format.hpp"
#include "Slic3r/Biz/Network/ServiceConfig.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/Assert.hpp"

#include <WebView2.h>
#include <wrl.h>
#include <atlbase.h>
#include <unordered_map>

#include <wx/msw/registry.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/log/trivial.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

#include <wx/webview.h>
#include <wrl/client.h>
#include <string>

namespace pt = boost::property_tree;
using Microsoft::WRL::ComPtr;

namespace Slic3r::App::WX::WebView {

std::unordered_map<ICoreWebView2*,EventRegistrationToken> g_basic_auth_handler_tokens;

void setup_webview_with_credentials(wxWebView* webview, const std::string& username, const std::string& password)
{
    ASSERT(webview);
    ICoreWebView2* webView2_raw = static_cast<ICoreWebView2*>(webview->GetNativeBackend());
    if (!webView2_raw) {
        SPDLOG_ERROR("{} Failed: Native WebView2 is null.", __FUNCTION__);
        return;
    }

    ComPtr<ICoreWebView2> wv2(webView2_raw);
    ComPtr<ICoreWebView2_10> wv2_10;
    HRESULT hr = wv2.As(&wv2_10);
    if (FAILED(hr)) {
        SPDLOG_ERROR("{} Failed: ICoreWebView2_10 interface not supported by runtime.", __FUNCTION__);
        return;
    }

    remove_webview_credentials(webview);

    EventRegistrationToken token = {};

    hr = wv2_10->add_BasicAuthenticationRequested(
        Microsoft::WRL::Callback<ICoreWebView2BasicAuthenticationRequestedEventHandler>(
            [username, password](ICoreWebView2* sender, ICoreWebView2BasicAuthenticationRequestedEventArgs* args) -> HRESULT {
                
                ComPtr<ICoreWebView2BasicAuthenticationResponse> response;
                HRESULT hr = args->get_Response(&response);
                if (FAILED(hr)) return hr;

                response->put_UserName(from_u8(username).c_str());
                response->put_Password(from_u8(password).c_str());
                
                return S_OK;
            }).Get(),
        &token
    );

    if (FAILED(hr)) {
        SPDLOG_ERROR("WebView: Cannot register authentication request handler. HRESULT: {0:x}", (unsigned int)hr);
    } else {
        g_basic_auth_handler_tokens[webView2_raw] = token;
    }
}

void remove_webview_credentials(wxWebView* webview)
{
    ASSERT(webview);
    ICoreWebView2* webView2_raw = static_cast<ICoreWebView2*>(webview->GetNativeBackend());
    if (!webView2_raw) {
        SPDLOG_ERROR("{} Failed: Native WebView2 is null.", __FUNCTION__);
        return;
    }

    ComPtr<ICoreWebView2> wv2(webView2_raw);
    ComPtr<ICoreWebView2_10> wv2_10;
    HRESULT hr = wv2.As(&wv2_10);
    if (FAILED(hr)) {
        SPDLOG_ERROR("{} Failed: ICoreWebView2_10 interface not supported by runtime.", __FUNCTION__);
        return;
    }

    if (auto it = g_basic_auth_handler_tokens.find(webView2_raw);
        it != g_basic_auth_handler_tokens.end()) {

        if (FAILED(wv2_10->remove_BasicAuthenticationRequested(it->second))) {
            SPDLOG_ERROR("WebView: Unregistering authentication request handler failed");
        } else {
            g_basic_auth_handler_tokens.erase(it);
        }
    } else {
        SPDLOG_ERROR("{}: Cannot unregister authentication request handler", __FUNCTION__);
    }

}
void delete_cookies(wxWebView* webview, const std::string& url)
{
    ICoreWebView2 *webView2 = static_cast<ICoreWebView2 *>(webview->GetNativeBackend());
    if (!webView2) {
        SPDLOG_ERROR("{} Failed: webView2 is null.", __FUNCTION__);
        return;
    }

    /*
    "cookies": [{
		"domain": ".google.com",
		"expires": 1756464458.304917,
		"httpOnly": true,
		"name": "__Secure-1PSIDCC",
		"path": "/",
		"priority": "High",
		"sameParty": false,
		"secure": true,
		"session": false,
		"size": 90,
		"sourcePort": 443,
		"sourceScheme": "Secure",
		"value": "AKEyXzUvV_KBqM4aOlsudROI_VZ-ToIH41LRbYJFtFjmKq_rOmx1owoyUGvQHbwr5be380fKuQ"
    },...]}
    */
    wxString parameters = format_wxstr(L"{\"urls\": [\"%1%\"]}", url);
    webView2->CallDevToolsProtocolMethod(L"Network.getCookies", parameters.c_str(),
        Microsoft::WRL::Callback<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
            [webView2, url](HRESULT errorCode, LPCWSTR resultJson) -> HRESULT {
                if (FAILED(errorCode)) {
                    return S_OK;
                }
                // Handle successful call (resultJson contains the list of cookies)
                pt::ptree ptree;
                try {
                    std::stringstream ss(into_u8(resultJson));
                    pt::read_json(ss, ptree);
                }
                catch (const std::exception& e) {
                    SPDLOG_ERROR("{}: Failed to parse cookies json: {}", __FUNCTION__, e.what());
                    return S_OK;
                }
                for (const auto& cookie : ptree.get_child("cookies")) {
                    std::string name = cookie.second.get<std::string>("name");
                    std::string domain = cookie.second.get<std::string>("domain");
                    // Delete cookie by name and domain
                    wxString name_and_domain = format_wxstr(L"{\"name\": \"%1%\", \"domain\": \"%2%\"}", name, domain);
                    //SPDLOG_DEBUG("Deleting cookie: {}", into_u8(name_and_domain));
                    webView2->CallDevToolsProtocolMethod(L"Network.deleteCookies", name_and_domain.c_str(),
                        Microsoft::WRL::Callback<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
                            [](HRESULT errorCode, LPCWSTR resultJson) -> HRESULT { return S_OK; }).Get());
                }       
                return S_OK;
            }
        ).Get());
    
}
void delete_cookies_with_counter(wxWebView* webview, const std::string& url, std::atomic<size_t>& counter)
{
    ICoreWebView2 *webView2 = static_cast<ICoreWebView2 *>(webview->GetNativeBackend());
    if (!webView2) {
        SPDLOG_ERROR("{} Failed: webView2 is null.", __FUNCTION__);
        return;
    }

    /*
    "cookies": [{
		"domain": ".google.com",
		"expires": 1756464458.304917,
		"httpOnly": true,
		"name": "__Secure-1PSIDCC",
		"path": "/",
		"priority": "High",
		"sameParty": false,
		"secure": true,
		"session": false,
		"size": 90,
		"sourcePort": 443,
		"sourceScheme": "Secure",
		"value": "AKEyXzUvV_KBqM4aOlsudROI_VZ-ToIH41LRbYJFtFjmKq_rOmx1owoyUGvQHbwr5be380fKuQ"
    },...]}
    */
    wxString parameters = format_wxstr(L"{\"urls\": [\"%1%\"]}", url);
    webView2->CallDevToolsProtocolMethod(L"Network.getCookies", parameters.c_str(),
        Microsoft::WRL::Callback<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
            [webView2, url, &counter](HRESULT errorCode, LPCWSTR resultJson) -> HRESULT {
                if (FAILED(errorCode)) {
                    return S_OK;
                }
                // Handle successful call (resultJson contains the list of cookies)
                pt::ptree ptree;
                try {
                    std::stringstream ss(into_u8(resultJson));
                    pt::read_json(ss, ptree);
                }
                catch (const std::exception& e) {
                    BOOST_LOG_TRIVIAL(error) << "Failed to parse cookies json: " << e.what();
                    return S_OK;
                }
                auto& cookies = ptree.get_child("cookies");
                if (cookies.size() == 0) {
                    counter++;
                    return S_OK;
                }
                for (auto it = cookies.begin(); it != cookies.end(); ++it) {
                    const auto& cookie = *it;
                    std::string name = cookie.second.get<std::string>("name");
                    std::string domain = cookie.second.get<std::string>("domain");
                    // Delete cookie by name and domain
                    wxString name_and_domain = format_wxstr(L"{\"name\": \"%1%\", \"domain\": \"%2%\"}", name, domain);
                    //SPDLOG_DEBUG("Deleting cookie: {}", into_u8(name_and_domain));
                    bool last = false;
                    if (std::next(it) == cookies.end()) {
                        last = true;
                    }
                    webView2->CallDevToolsProtocolMethod(L"Network.deleteCookies", name_and_domain.c_str(),
                        Microsoft::WRL::Callback<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
                            [last, &counter](HRESULT errorCode, LPCWSTR resultJson) -> HRESULT { 
                                if (last) {
                                    counter++;
                                }
                                return S_OK; 
                            }).Get());
                }       
                return S_OK;
            }
        ).Get());
    
}
static EventRegistrationToken m_webResourceRequestedTokenForImageBlocking = {};
static wxString filter_patern;
namespace {
void RequestHeadersToLog(ICoreWebView2HttpRequestHeaders* requestHeaders)
{
    if (!requestHeaders) return;

    ComPtr<ICoreWebView2HttpHeadersCollectionIterator> iterator;
    HRESULT hr = requestHeaders->GetIterator(&iterator);
    
    if (FAILED(hr)) return;

    BOOL hasCurrent = FALSE;
    iterator->get_HasCurrentHeader(&hasCurrent);

    SPDLOG_INFO("Logging request headers:");

    while (hasCurrent)
    {
        LPWSTR name = nullptr;
        LPWSTR value = nullptr;

        // GetCurrentHeader allocates memory via CoTaskMemAlloc
        hr = iterator->GetCurrentHeader(&name, &value);
        
        if (SUCCEEDED(hr)) {
            //SPDLOG_INFO("Header: {} = {}", name ? name : L"NULL", value ? value : L"NULL");

            if (name) CoTaskMemFree(name);
            if (value) CoTaskMemFree(value);
        }

        BOOL hasNext = FALSE;
        iterator->MoveNext(&hasNext);
        hasCurrent = hasNext;
    }
}
}

void add_request_authorization(wxWebView* webview, const wxString& address, const std::string& token)
{
    // This function adds a filter so when pattern document is being requested, callback is triggered
    // Inside add_WebResourceRequested callback, there is a Authorization header added.
    // The filter needs to be removed to stop adding the auth header

    ASSERT(webview);
    ICoreWebView2* webView2_raw = static_cast<ICoreWebView2*>(webview->GetNativeBackend());
    if (!webView2_raw) {
        SPDLOG_ERROR("setup_webview_with_credentials Failed: Native WebView2 is null.");
        return;
    }

    filter_patern =  address + L"/*";
    webView2_raw->AddWebResourceRequestedFilter( filter_patern.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT);
    
    if (FAILED(webView2_raw->add_WebResourceRequested(
            Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [token](ICoreWebView2 *sender, ICoreWebView2WebResourceRequestedEventArgs *args) {
                    // Get the web resource request
                    ComPtr<ICoreWebView2WebResourceRequest> request;
                    HRESULT hr = args->get_Request(&request);
                    if (FAILED(hr))
                    {
                        SPDLOG_ERROR("Adding request authorization failed: Failed to get_Request.");
                        return S_OK;
                    }
                    // Get the request headers
                    ComPtr<ICoreWebView2HttpRequestHeaders> headers;
                    hr = request->get_Headers(&headers);
                    if (FAILED(hr)) {
                         SPDLOG_ERROR("Adding request authorization failed: Failed to get_Headers.");
                         return S_OK;
                    }

                    std::string val = "External " + token;
                    // Add or modify the Authorization header
                    hr = headers->SetHeader(L"Authorization", from_u8(val).c_str());

                    // This function is only needed for debug purpose
                    //RequestHeadersToLog(headers.Get());    
                    return S_OK;
                }
            ).Get(), &m_webResourceRequestedTokenForImageBlocking
            ))) {

        SPDLOG_ERROR("{}: Failed to add callback.", __FUNCTION__);
    }
}

void remove_request_authorization(wxWebView* webview)
{
    ICoreWebView2 *webView2 = static_cast<ICoreWebView2 *>(webview->GetNativeBackend());
    if (!webView2) {
        SPDLOG_ERROR("{} Failed: webView2 is null.", __FUNCTION__);
        return;
    }
    webView2->RemoveWebResourceRequestedFilter(filter_patern.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT);
    if(FAILED(webView2->remove_WebResourceRequested( m_webResourceRequestedTokenForImageBlocking))) {
        SPDLOG_ERROR("{}: Failed to remove resources", __FUNCTION__);
    }
}

void load_request(wxWebView* web_view, const std::string& address, const std::string& token)
{
    // This function should create its own GET request and send it (works on linux)
    // For that we would use NavigateWithWebResourceRequest.
    // For that we need ICoreWebView2Environment smart pointer.
    // Such pointer does exists inside wxWebView edge backend. (wxWebViewEdgeImpl::m_webViewEnvironment)
    // But its currently private and not getable. (It wouldn't be such problem to create the getter)
    
    PANIC("Not implemented for Wi32.");

    //ICoreWebView2 *webView2 = static_cast<ICoreWebView2 *>(web_view->GetNativeBackend());
    //if (!webView2) {
    //    SPDLOG_ERROR("load_request Failed: webView2 is null.");
    //    return;
    //}
   
    //// GetEnviroment does not exists
    //ComPtr<ICoreWebView2Environment> webViewEnvironment;
    ////webViewEnvironment = static_cast<ICoreWebView2Environment *>(web_view->GetEnviroment());
    //if (!webViewEnvironment.Get()) {
    //    SPDLOG_ERROR("load_request Failed: ICoreWebView2Environment is null.");
    //    return;
    //}

    //ComPtr<ICoreWebView2Environment2> webViewEnvironment2;
    //if (FAILED(webViewEnvironment->QueryInterface(IID_PPV_ARGS(&webViewEnvironment2))))
    //{
    //    SPDLOG_ERROR("load_request Failed: ICoreWebView2Environment2 is null.");
    //    return;
    //}
    // ComPtr<ICoreWebView2WebResourceRequest> webResourceRequest;
    //
    //wxString printables_address = from_u8(Biz::Network::ServiceConfig::instance().printables_url());
    //if (FAILED(webViewEnvironment2->CreateWebResourceRequest(
    //    printables_address.wc_str(), L"GET", NULL,
    //    L"Content-Type: application/x-www-form-urlencoded", &webResourceRequest)))
    //{
    //    SPDLOG_ERROR("load_request Failed: CreateWebResourceRequest failed.");
    //    return;
    //}

    //ComPtr<ICoreWebView2> wv2(webView2);
    //ComPtr<ICoreWebView2_2> wv2_2;
    //HRESULT hr = wv2.As(&wv2_2);
    //if (FAILED(hr)) {
    //    SPDLOG_ERROR("setup_webview_with_credentials Failed: ICoreWebView2_10 interface not supported by runtime.");
    //    return;
    //}

    //if (FAILED(wv2_2->NavigateWithWebResourceRequest(webResourceRequest.Get())))
    //{
    //    SPDLOG_ERROR("load_request Failed: NavigateWithWebResourceRequest failed.");
    //    return;
    //} 
}

void register_prusaslicer_url()
{
    boost::filesystem::path binary_path(boost::filesystem::canonical(boost::dll::program_location()));
    // the path to binary needs to be correctly saved in string with respect to localized characters
    wxString key_wstring = L"\"" + from_u8(binary_path.string()) + L"\" \"--single-instance\" \"%1\"";
    SPDLOG_INFO("Downloader registration: Path of binary: {}", into_u8(key_wstring));
    wxRegKey key_first(wxRegKey::HKCU, L"Software\\Classes\\prusaslicer");
    wxRegKey key_full(wxRegKey::HKCU, L"Software\\Classes\\prusaslicer\\shell\\open\\command");
    if (!key_first.Exists()) {
        key_first.Create(false);
    }
    key_first.SetValue(L"URL Protocol", L"");
    if (!key_full.Exists()) {
        key_full.Create(false);
    }
    bool success = key_full.SetValue(L"", key_wstring);

    if (!success) {
        SPDLOG_ERROR("Failed to write registry value.");
    }
}

} // Slic3r::App::WX::WebView
