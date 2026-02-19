#include "Slic3r/Biz/Network/HttpCurlFactory.hpp"

#include "Slic3r/Biz/Network/HttpFactory.hpp"
#include "Slic3r/Biz/Network/HttpCurl.hpp"


namespace Slic3r::Biz::Network {

void HttpFactory::initialize()
{
    configure_http_factory_with_curl();
}

void configure_http_factory_with_curl() {
    auto& factory = HttpFactory::instance();

    factory.set_create_fn([](IHttp::RequestMethod request_method, std::string url, IHttp::RetryFn fn) -> std::unique_ptr<IHttp> {
        return std::make_unique<HttpCurl>(request_method, std::move(url), std::move(fn));
    });

    factory.set_extract_host_from_url_fn([](const std::string& url) -> std::string {
        return HttpCurl::extract_host_from_url(url);
    });

    factory.set_substitute_host_fn([](const std::string& orig_addr, std::string sub_addr) -> std::string {
        return HttpCurl::substitute_host(orig_addr, std::move(sub_addr));
    });

    factory.set_escape_path_by_element_fn([](const boost::filesystem::path& path) -> std::string {
        return HttpCurl::escape_path_by_element(path);
    });

    factory.set_escape_string_fn([](const std::string& str) -> std::string {
        return HttpCurl::escape_string(str);
    });

    factory.set_unescape_string_fn([](const std::string& str) -> std::string {
        return HttpCurl::unescape_string(str);
    });

    factory.set_is_subdomain_fn([](const std::string& url, const std::string& domain) -> bool {
        return HttpCurl::is_subdomain(url, domain);
    });

    factory.set_get_apex_domain_fn([](const std::string& url) -> std::string {
        return HttpCurl::get_apex_domain(url);
    });

    factory.set_ca_file_supported_fn([]() -> bool {
        return HttpCurl::ca_file_supported();
    });

    factory.set_tls_global_init_fn([]() -> std::string {
        return HttpCurl::tls_global_init();
    });

    factory.set_tls_system_cert_store_fn([]() -> std::string {
        return HttpCurl::tls_system_cert_store();
    });
}
}