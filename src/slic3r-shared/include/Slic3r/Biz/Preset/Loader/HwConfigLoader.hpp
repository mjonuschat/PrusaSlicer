#pragma once

#include <Slic3r/Domain/Preset/HwConfig.hpp>
namespace Slic3r::Biz::Preset::Loader {



class HwConfigLoader
{
public:
    HwConfigLoader();
    Domain::Preset::VendorData& load(const std::string & filename);

    const Domain::Preset::VendorData& result() const { return m_result; }
    Domain::Preset::VendorData& result() { return m_result; }

private:
    Domain::Preset::VendorData m_result;
};

}
