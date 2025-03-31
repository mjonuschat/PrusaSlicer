#pragma once

#include "Slic3r/Biz/Network/IHttp.hpp"

#include <memory>

namespace Slic3r::Biz::Network::HttpFactory {
/**
 * @brief This factory exists to support multiple IHttp implementations (in case libcurl is not available).
 */
std::unique_ptr<IHttp> create(IHttp::RequestMethod request_method, std::string url, IHttp::RetryFn fn);


// Static helper functions.
std::string extract_host_from_url(const std::string& url_in);
std::string substitute_host(const std::string& orig_addr, std::string sub_addr);
std::string escape_path_by_element(const boost::filesystem::path& path);
std::string escape_string(const std::string& str);
bool ca_file_supported();
std::string tls_global_init();
std::string tls_system_cert_store();
} // namespace Slic3r::Biz::Network::HttpFactory