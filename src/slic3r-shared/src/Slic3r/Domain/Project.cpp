#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Biz/Config/3mf_legacy.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"
#include "Slic3r/Domain/Model.hpp"

namespace Slic3r::Domain {

using Biz::Platform::PlatformServices;
using Biz::JThread::StopToken;

Project::Project() : m_model(new Model()) {}

void Project::load(const std::string& file_path, std::function<void(Project&&)> after_load)
{
    PlatformServices::instance().job_manager().create_job("project_load", [](StopToken stop_token, const std::string file_path){
        Domain::ConfigPack config;
        WipeTowersOnBeds wipe_towers;
        CustomGCodesOnBeds custom_gcodes;
        Model model;

        ConfigSubstitutionContext context{ForwardCompatibilitySubstitutionRule::Disable};
        boost::optional<Semver> version;

        ASSERT(Slic3rLegacy::load_3mf_legacy(
            file_path.c_str(),
            config,
            &model,
            false,
            version,
            wipe_towers,
            custom_gcodes
        ));

        Project project;
        project.m_model = std::make_unique<Model>(std::move(model));
        project.set_file_name(file_path);
        // TODO: implement
        project.m_config_containers.clear();
        project.m_config_containers.emplace_back(std::make_unique<ConfigContainer>());
        auto& config_container = project.m_config_containers.back();
        DynamicPrintConfig co;
        auto full{FullPrintConfig::defaults()};
        co.apply(full);
        config_container->set_print_config(co);
        config_container->set_print_config_new(config);
        //config_container->set_bed(m_bed_container.add_bed())
        //ASSERT(config_container->bed_instances().size() == wipe_towers.size());

        return project;

    }, file_path).on_result(after_load).start();
}

const ConfigContainer* Project::find_config_container(size_t id) const
{
    return find_by_id<ConfigContainer>(m_config_containers, id);
}

ConfigContainer* Project::find_config_container(size_t id)
{
    return find_by_id<ConfigContainer>(m_config_containers, id);
}

const Domain::ModelObject* Project::find_object_by_id(size_t id) const
{
    return find_by_id<Domain::ModelObject>(m_model->objects, id);
}

const Domain::ModelVolume* Project::find_volume_by_id(size_t obj_id, size_t vol_id) const
{
    const auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<Domain::ModelVolume>(obj->volumes, vol_id);
}

const ModelInstance* Project::find_instance_by_id(size_t obj_id, size_t inst_id) const
{
    const auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<ModelInstance>(obj->instances, inst_id);
}

Domain::ModelObject* Project::find_object_by_id(size_t id)
{
    return find_by_id<Domain::ModelObject>(m_model->objects, id);
}

Domain::ModelVolume* Project::find_volume_by_id(size_t obj_id, size_t vol_id)
{
    auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<Domain::ModelVolume>(obj->volumes, vol_id);
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
