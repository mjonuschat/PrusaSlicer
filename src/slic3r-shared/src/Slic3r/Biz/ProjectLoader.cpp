#include "Slic3r/Biz/ProjectLoader.hpp"
#include "Slic3r/Biz/Config/3mf_legacy.hpp"
#include "Slic3r/Biz/Format/3mf.hpp"

#include "Slic3r/Assert.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"

namespace Slic3r::Biz {

static Domain::Project load_legacy_project(const std::string& file_path)
{
    Domain::Project project;

    Domain::ConfigPack config;
    Domain::WipeTowersOnBeds wipe_towers;
    Domain::CustomGCodesOnBeds custom_gcodes;
    boost::optional<Semver> version;

    if (! Slic3rLegacy::load_3mf_legacy(file_path.c_str(), config, &project.model(),
        false, version, wipe_towers, custom_gcodes))
        throw std::runtime_error("Loading of legacy 3MF failed.");

    project.set_file_name(file_path);
    // TODO: implement
    project.config_containers().clear();
    project.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
    auto& config_container = project.config_containers().back();
    config_container->set_print_config_new(config);
    DynamicPrintConfig co;
    auto full{FullPrintConfig::defaults()};
    co.apply(full);
    config_container->set_print_config(co);
    return project;
}

void load_project(const std::string& file_path, std::function<void(Domain::Project&&)> after_load)
{
    Platform::PlatformServices::instance().job_manager().create_job("project_load", [](Biz::JThread::StopToken stop_token, const std::string file_path) {

        Domain::Project project;

        try {
            if (load_3mf(file_path, project))
                return project;
        } catch (const Old3MFException&) {
            return load_legacy_project(file_path);
        }

        return project; // for now, return empty project in case of failure
    }, file_path).on_result(after_load).start();
}

} // namespace Slic3r::Biz
