#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Biz/Config/3mf_legacy.hpp"

#include <libslic3r/Model.hpp>

namespace Slic3r::Domain {

Project::Project() : m_model(new Model()) {}

void Project::load(const std::string& file_path)
{
    Domain::ConfigPack config;
    WipeTowersOnBeds wipe_towers;
    CustomGCodesOnBeds custom_gcodes;

    ConfigSubstitutionContext context{ForwardCompatibilitySubstitutionRule::Disable};
    boost::optional<Semver> version;

    ASSERT(Slic3rLegacy::load_3mf_legacy(
        file_path.c_str(),
        config,
        m_model.get(),
        false,
        version,
        wipe_towers,
        custom_gcodes
    ));

    set_file_name(file_path);
    // TODO: implement
    m_config_containers.clear();
    m_config_containers.emplace_back(std::make_unique<ConfigContainer>());
    auto& config_container = m_config_containers.back();
    config_container->set_print_config_new(config);
    DynamicPrintConfig co;
    auto full{FullPrintConfig::defaults()};
    co.apply(full);
    config_container->set_print_config(co);
    //config_container->set_bed(m_bed_container.add_bed())
    //ASSERT(config_container->bed_instances().size() == wipe_towers.size());

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
