#pragma once
#include <vector>
#include "ConfigContainer.hpp"


namespace Slic3r::Domain {

/**
 * All data that can be loaded/saved into .3mf file.
 */
class Project
{
public:
    using ConfigContainerList = std::vector<std::unique_ptr<ConfigContainer>>;

    [[nodiscard]] ConfigContainerList& config_contianers() { return m_config_containers; }
    [[nodiscard]] const ConfigContainerList& config_containers() const { return m_config_containers; }

private:
    ConfigContainerList m_config_containers;
};

} // namespace Slic3r::Domain