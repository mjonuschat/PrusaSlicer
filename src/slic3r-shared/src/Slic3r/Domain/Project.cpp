#include "Slic3r/Domain/Project.hpp"

#include "libslic3r/NSVGUtils.hpp"

#include <boost/geometry/index/detail/algorithms/bounds.hpp>
#include <libslic3r/Model.hpp>
#include <libslic3r/FileReader.hpp>

namespace Slic3r::Domain {

Project::Project() : m_model(new Model()) {}

void Project::load(const std::string& file_path)
{
    m_model = std::make_unique<Model>(FileReader::load_model(file_path));
    set_file_name(file_path);
    // TODO: implement
    /*
    m_config_containers.clear();
    m_config_containers.emplace_back();
    auto& config_container = m_config_containers.back();
    */
}

const ConfigContainer* Project::find_config_container(size_t id) const
{
    return find_by_id<ConfigContainer>(m_config_containers, id);
}

ConfigContainer* Project::find_config_container(size_t id)
{
    return find_by_id<ConfigContainer>(m_config_containers, id);
}

const ModelObject* Project::find_object_by_id(size_t id) const
{
    return find_by_id<ModelObject>(m_model->objects, id);
}

const ModelVolume* Project::find_volume_by_id(size_t obj_id, size_t vol_id) const
{
    const auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<ModelVolume>(obj->volumes, vol_id);
}

const ModelInstance* Project::find_instance_by_id(size_t obj_id, size_t inst_id) const
{
    const auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<ModelInstance>(obj->instances, inst_id);
}

ModelObject* Project::find_object_by_id(size_t id)
{
    return find_by_id<ModelObject>(m_model->objects, id);
}

ModelVolume* Project::find_volume_by_id(size_t obj_id, size_t vol_id)
{
    auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<ModelVolume>(obj->volumes, vol_id);
}

ModelInstance* Project::find_instance_by_id(size_t obj_id, size_t inst_id)
{
    auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<ModelInstance>(obj->instances, inst_id);
}

const BedInstance* Project::find_bed_instance_by_id(size_t id) const
{
    for (const auto& cc : m_config_containers)
        if (auto* bed_inst = find_by_id(cc->bed_instances(), id))
            return bed_inst;
    return nullptr;
}

BedInstance* Project::find_bed_instance_by_id(size_t id)
{
    for (const auto& cc : m_config_containers)
        if (auto* bed_inst = find_by_id(cc->bed_instances(), id))
            return bed_inst;
    return nullptr;
}

const Bed* Project::find_bed_by_id(size_t id) const
{
    return find_by_id(m_bed_container.beds(), id);
}

Bed* Project::find_bed_by_id(size_t id)
{
    return find_by_id(m_bed_container.beds(), id);
}


} // namespace Slic3r::Domain
