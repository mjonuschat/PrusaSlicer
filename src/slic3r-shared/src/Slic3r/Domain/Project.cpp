#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "libslic3r/Model.hpp"

namespace Slic3r::Domain {

Project::Project() : m_model(new Model()) {}

void Project::load(const std::string& file_path)
{
    m_model = std::make_unique<Model>(Model::read_from_file(file_path));
    set_file_name(file_path);
    m_config_containers.clear();
    m_config_containers.emplace_back();
    auto& config_container = m_config_containers.back();

}

}
