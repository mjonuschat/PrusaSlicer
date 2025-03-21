#pragma once

#include <Slic3r/Domain/Preset/HwConfig.hpp>
namespace Slic3r::Biz::Preset::Loader {



class HwConfigLoader
{
public:
    HwConfigLoader();
    Domain::Preset::HwDefs& load(const std::string & filename);

    const Domain::Preset::HwDefs& result() const { return m_result; }
    Domain::Preset::HwDefs& result() { return m_result; }

private:
    Domain::Preset::HwDefs m_result;
};

}
