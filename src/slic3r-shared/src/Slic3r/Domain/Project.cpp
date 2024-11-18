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

const ModelObject* Project::find_object_by_id(size_t id) const
{
//    auto it = std::find_if(m_model->objects.begin(), m_model->objects.end(), [id](const auto& obj) {
//        return obj->id() == id;
//    });
//    return it == m_model->objects.end() ? nullptr : *it;
    return find_by_id<ModelObject>(m_model->objects, id);
}

const ModelVolume* Project::find_volume_by_id(size_t obj_id, size_t vol_id) const
{
    const auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
//    auto it = std::find_if(obj->volumes.begin(), obj->volumes.end(), [vol_id](const auto& vol) {
//        return vol->id() == vol_id;
//    });
//    return it == obj->volumes.end() ? nullptr : *it;
    return find_by_id<ModelVolume>(obj->volumes, vol_id);
}

const ModelInstance* Project::find_instance_by_id(size_t obj_id, size_t inst_id) const
{
    const auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
//    auto it = std::find_if(obj->instances.begin(), obj->instances.end(), [inst_id](const auto& inst) {
//        return inst->id() == inst_id;
//    });
//    return it == obj->instances.end() ? nullptr : *it;
    return find_by_id<ModelInstance>(obj->instances, inst_id);
}

ModelObject* Project::find_object_by_id(size_t id)
{
//    auto it = std::find_if(m_model->objects.begin(), m_model->objects.end(), [id](const auto& obj) {
//        return obj->id() == id;
//    });
//    return it == m_model->objects.end() ? nullptr : *it;
    return find_by_id<ModelObject>(m_model->objects, id);
}

ModelVolume* Project::find_volume_by_id(size_t obj_id, size_t vol_id)
{
    auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
//    auto it = std::find_if(obj->volumes.begin(), obj->volumes.end(), [vol_id](const auto& vol) {
//        return vol->id() == vol_id;
//    });
//    return it == obj->volumes.end() ? nullptr : *it;
    return find_by_id<ModelVolume>(obj->volumes, vol_id);
}

ModelInstance* Project::find_instance_by_id(size_t obj_id, size_t inst_id)
{
    auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
//    auto it = std::find_if(obj->instances.begin(), obj->instances.end(), [inst_id](const auto& inst) {
//        return inst->id() == inst_id;
//    });
//    return it == obj->instances.end() ? nullptr : *it;
    return find_by_id<ModelInstance>(obj->instances, inst_id);
}


} // namespace Slic3r::Domain
