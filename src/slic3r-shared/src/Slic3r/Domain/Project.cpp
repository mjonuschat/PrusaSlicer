#include "Project.hpp"
#include "ConfigContainer.hpp"
#include "Bed.hpp"
#include "libslic3r/Model.hpp"

namespace Slic3r::Domain {

void Project::load(const std::string& file_path)
{
    m_model = std::make_unique<Model>(Model::read_from_file(file_path));
    set_file_name(file_path);
    m_config_containers.clear();
    m_config_containers.emplace_back();
    auto& config_container = m_config_containers.back();

}

}
