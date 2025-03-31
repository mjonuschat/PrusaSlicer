#include "Slic3r/Biz/Network/HttpFactory.hpp"
#include "Slic3r/Biz/Network/HttpCurl.hpp"


namespace Slic3r::Biz::Network::HttpFactory {
std::unique_ptr<IHttp> create(IHttp::RequestMethod request_method, std::string url, IHttp::RetryFn fn)
{
    return std::make_unique<HttpCurl>(request_method, std::move(url), std::move(fn));
}

std::string extract_host_from_url(const std::string& url)
{
    return HttpCurl::extract_host_from_url(url);
}
std::string substitute_host(const std::string& orig_addr, std::string sub_addr)
{
    return HttpCurl::substitute_host(orig_addr, sub_addr);
}

std::string escape_path_by_element(const boost::filesystem::path& path)
{
    return HttpCurl::escape_path_by_element(path);
}

std::string escape_string(const std::string& str)
{
    return HttpCurl::escape_string(str);
}

bool ca_file_supported()
{
    return HttpCurl::ca_file_supported();
}
std::string tls_global_init()
{
    return HttpCurl::tls_global_init();
}
std::string tls_system_cert_store()
{
    return HttpCurl::tls_system_cert_store();
}
}