#pragma once
#include <string>
#include <vector>
#include <memory>
#include "ConfigContainer.hpp"

namespace Slic3r { class Model; }

namespace Slic3r::Domain {

/**
 * All data that can be loaded/saved into .3mf file.
 */
class Project
{
public:
    using ConfigContainerList = std::vector<std::unique_ptr<ConfigContainer>>;

    void load(const std::string& file_path);

    void set_file_name(const std::string& file_name) { m_file_name = file_name; }
    [[nodiscard]] const std::string& file_name() const { return m_file_name; }

    [[nodiscard]] ConfigContainerList& config_containers() { return m_config_containers; }
    [[nodiscard]] const ConfigContainerList& config_containers() const { return m_config_containers; }

private:
    std::string m_file_name;
    ConfigContainerList m_config_containers;
    std::unique_ptr<Model> m_model;
};

} // namespace Slic3r::Domain