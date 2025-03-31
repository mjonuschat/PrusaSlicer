#pragma once

#include <Slic3r/Biz/PrintHost/PrintHostPrusaLink.hpp>

namespace boost::filesystem { class path; }

namespace Slic3r::Biz::PrintHost {

class PrintHostSL1Host : public PrintHostPrusaLink {

public:
    PrintHostSL1Host(PrintHostConfig config) : PrintHostPrusaLink(std::move(config)) {}
    
    PrintHostSL1Host(const PrintHostSL1Host&) = delete;
    PrintHostSL1Host& operator=(const PrintHostSL1Host&) = delete;
    PrintHostSL1Host(PrintHostSL1Host&& other) noexcept = default;
    PrintHostSL1Host& operator=(PrintHostSL1Host&& other) noexcept = default;
   
    ~PrintHostSL1Host() override {}

    const char* get_name() const override { return "SL1Host"; }

protected:
    bool validate_version_text(const boost::optional<std::string>& version_text) const override;

};
} // namespace Slic3r::Biz::PrintHost